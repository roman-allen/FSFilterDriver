using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Management;
using Microsoft.Win32;

namespace FSFilterDriverAPI
{
    public class FSFilterDriverAPI
    {
        private Int32 portHandle;
        private const string portName = "\\FSFilterDriverPort";
        private const string lockedFilesRegistryPath = @"SYSTEM\ControlSet001\Services\FSFilterDriver\LockedFiles";

        enum DriverCommand
        {
            COMMUNICATION_COMMAND_SET_WHITELIST = 0x1000,
            COMMUNICATION_COMMAND_GET_REPORTED_ITEMS = 0x2000,
            COMMUNICATION_COMMAND_LOCKED_FILES_UPDATED = 0x3000,
        }

        public bool IsConnected
        {
            get
            {
                return portHandle != 0;
            }
        }

        public FSFilterDriverAPI()
        {
        }

        public void Connect()
        {
            if (portHandle != 0)
            {
                throw new FSFilterDriverAPIException("Connection is opened already");
            }

            IntPtr portHandlePtr = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(IntPtr)));
            int status = FilterConnectCommunicationPort(portName, 0, IntPtr.Zero, 0, IntPtr.Zero, portHandlePtr);
            if (status != 0)
            {
                Marshal.FreeHGlobal(portHandlePtr);
                throw new FSFilterDriverAPIException(string.Format("Error connecting to driver port: {0:X}", status));
            }
            portHandle = Marshal.ReadInt32(portHandlePtr);
            Marshal.FreeHGlobal(portHandlePtr);
        }

        public void Disconnect()
        {
            if (portHandle == 0)
            {
                throw new FSFilterDriverAPIException("Connection doesn't exist");
            }

            if (CloseHandle(portHandle) == 0)
            {
                throw new FSFilterDriverAPIException("Error closing communication port");
            }
        }


        public void SendProcessesWhiteList(List<string> processesList)
        {
            byte[] data;

            if (portHandle == 0)
            {
                throw new FSFilterDriverAPIException("Connection must be established before calling this method");
            }

            using (MemoryStream ms = new MemoryStream())
            {
                using (BinaryWriter bw = new BinaryWriter(ms, System.Text.Encoding.Unicode))
                {
                    // command - first 32 bit
                    bw.Write((Int32)DriverCommand.COMMUNICATION_COMMAND_SET_WHITELIST);

                    bool first = true;
                    foreach (string procName in processesList)
                    {
                        // write block delimiter
                        if (!first) bw.Write((ushort)0xFFFF);
                        
                        // write process name
                        bw.Write(System.Text.Encoding.Unicode.GetBytes(procName));
                        first = false;
                    }

                    // write end of data
                    bw.Write((ushort)0x0000);
                    bw.Flush();

                    ms.Seek(0, SeekOrigin.Begin);
                    data = ms.ToArray();
                }
            }

            IntPtr command = Marshal.AllocHGlobal(data.Length);
            if (command == IntPtr.Zero)
            {
                throw new FSFilterDriverAPIException("Memory allocation error");
            }

            try {
                Marshal.Copy(data, 0, command, data.Length);

                uint bytesReturned = 0;
                int status = FilterSendMessage(portHandle, command, (uint)data.Length, IntPtr.Zero, 0, out bytesReturned);
                if (status != 0)
                {
                    throw new FSFilterDriverAPIException(string.Format("Error sending message to driver: {0:X}", status));
                }
            } finally
            {
                Marshal.FreeHGlobal(command);
            }
        }

        public List<string> GetReportedItems()
        {
            if (portHandle == 0)
            {
                throw new FSFilterDriverAPIException("Connection must be established before calling this method");
            }

            IntPtr command = Marshal.AllocHGlobal(sizeof(Int32));
            if (command == IntPtr.Zero)
            {
                throw new FSFilterDriverAPIException("Memory allocation error");
            }
            Marshal.WriteInt32(command, (Int32)DriverCommand.COMMUNICATION_COMMAND_GET_REPORTED_ITEMS);

            uint bufferSize = 32768;
            IntPtr buffer = Marshal.AllocHGlobal((int)bufferSize);
            if (command == IntPtr.Zero)
            {
                throw new FSFilterDriverAPIException("Memory allocation error");
            }

            uint bytesReturned = 0;
            int status = FilterSendMessage(portHandle, command, sizeof(Int32), buffer, bufferSize, out bytesReturned);
            if (status != 0)
            {
                throw new FSFilterDriverAPIException(string.Format("Error sending message to driver: {0:X}", status));
            }

            if (bytesReturned == 0)
            {
                return new List<string>();
            }

            byte[] data = new byte[bytesReturned];
            Marshal.Copy(buffer, data, 0, (int)bytesReturned);

            List<string> result = new List<string>();

            // check if we get more than terminating 0x0000
            if (bytesReturned > sizeof(char))
            {
                int index = 0;
                StringBuilder sb = new StringBuilder();
                while (index < data.Length)
                {
                    char ch = System.Text.Encoding.Unicode.GetChars(data, index, sizeof(char))[0];
                    if (ch == 0xFFFF || ch == 0x0000)
                    {
                        result.Add(sb.ToString());
                        index += sizeof(char);
                        sb = new StringBuilder();
                        continue;
                    }
                    index += sizeof(char);
                    sb.Append(ch);
                }
            }

            return result;
        }

        public void SendLockedFilesListUpdatedNotification()
        {
            if (portHandle == 0)
            {
                throw new FSFilterDriverAPIException("Connection must be established before calling this method");
            }

            IntPtr command = Marshal.AllocHGlobal(sizeof(Int32));
            if (command == IntPtr.Zero)
            {
                throw new FSFilterDriverAPIException("Memory allocation error");
            }
            Marshal.WriteInt32(command, (Int32)DriverCommand.COMMUNICATION_COMMAND_LOCKED_FILES_UPDATED);

            uint bytesReturned = 0;
            int status = FilterSendMessage(portHandle, command, sizeof(Int32), IntPtr.Zero, 0, out bytesReturned);

            Marshal.FreeHGlobal(command);

            if (status != 0)
            {
                throw new FSFilterDriverAPIException(string.Format("Error sending message to driver: {0:X}", status));
            }
        }

        public void LockFiles(List<string> files, bool notifyDriver)
        {
            RegistryKey key = Registry.LocalMachine.OpenSubKey(lockedFilesRegistryPath, true);
            if (key == null)
            {
                key = Registry.LocalMachine.CreateSubKey(lockedFilesRegistryPath);
            }
            if (key == null)
            {
                throw new FSFilterDriverAPIException("Cannot open registry key " + lockedFilesRegistryPath);
            }
            try {
                var valueNames = key.GetValueNames().ToList();
                var existingPairs = valueNames.Select(x => new { name = x, value = key.GetValue(x) as string }).ToList();
                int maxIndex = existingPairs.Any() ? existingPairs.Select(x => int.Parse(x.name)).Max() + 1 : 1;
                foreach (string newFile in files)
                {
                    if (!existingPairs.Any(pair => String.Equals(newFile, pair.value, StringComparison.InvariantCultureIgnoreCase)))
                    {
                        key.SetValue((maxIndex++).ToString("D5"), newFile);
                    }
                }

                if (notifyDriver)
                {
                    SendLockedFilesListUpdatedNotification();
                }
            }
            finally
            {
                key.Close();
            }
        }

        public List<string> GetLockedFiles()
        {
            RegistryKey key = Registry.LocalMachine.OpenSubKey(lockedFilesRegistryPath, true);
            if (key == null)
            {
                return new List<string>();
            }
            try
            {
                return key.GetValueNames().Select(x => key.GetValue(x) as string).ToList();
            }
            finally
            {
                key.Close();
            }
        }

        public void UnlockFiles(List<string> files, bool notifyDriver)
        {
            RegistryKey key = Registry.LocalMachine.OpenSubKey(lockedFilesRegistryPath, true);
            if (key == null)
            {
                key = Registry.LocalMachine.CreateSubKey(lockedFilesRegistryPath);
            }
            if (key == null)
            {
                throw new FSFilterDriverAPIException("Cannot open registry key " + lockedFilesRegistryPath);
            }
            try
            {
                var valueNames = key.GetValueNames().ToList();
                // skip removing file names from registry if no files are locked
                if (valueNames.Any())
                {
                    var existingPairs = valueNames.Select(x => new { name = x, value = key.GetValue(x) as string }).ToList();
                    int maxIndex = existingPairs.Select(x => int.Parse(x.name)).Max() + 1;
                    foreach (string unlockFile in files)
                    {
                        var unlockPair = existingPairs.FirstOrDefault(pair => String.Equals(unlockFile, pair.value, StringComparison.InvariantCultureIgnoreCase));
                        if (unlockPair != null)
                        {
                            try {
                                key.DeleteValue(unlockPair.name);
                            } catch
                            {
                                // ignore this because there is a chance that key was removed because many files are shared among different drivers
                            }
                        }
                    }
                }
                // tell driver that list of locked files has changed
                if (notifyDriver)
                {
                    SendLockedFilesListUpdatedNotification();
                }
            }
            finally
            {
                key.Close();
            }
        }


        [DllImport("fltlib.dll", SetLastError = true)]
        public static extern int FilterConnectCommunicationPort
            ([MarshalAs(UnmanagedType.LPWStr)]
                    string portName,
            uint options,
            IntPtr context,
            uint sizeOfContext,
            IntPtr securityAttributes,
            IntPtr hPort);

        [DllImport("fltlib.dll", SetLastError = true)]
        public static extern int FilterSendMessage(
            Int32 hPort,
            IntPtr inBuffer,
            UInt32 inBufferSize,
            IntPtr outBuffer,
            UInt32 outBufferSize,
            out UInt32 bytesReturned);

        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern int CloseHandle(Int32 hObject);

        [DllImport("kernel32.dll", EntryPoint = "FindFirstVolume", SetLastError = true, CallingConvention = CallingConvention.StdCall)]
        public static extern int FindFirstVolume(StringBuilder lpszVolumeName, int cchBufferLength);

        [DllImport("kernel32.dll", EntryPoint = "FindNextVolume", SetLastError = true, CallingConvention = CallingConvention.StdCall)]
        public static extern bool FindNextVolume(int hFindVolume, StringBuilder lpszVolumeName, int cchBufferLength);

        [DllImport("kernel32.dll", EntryPoint = "FindVolumeClose", SetLastError = true, CallingConvention = CallingConvention.StdCall)]
        public static extern bool FindVolumeClose(int hFindVolume);

        [DllImport("kernel32.dll", EntryPoint = "QueryDosDevice", SetLastError = true, CallingConvention = CallingConvention.StdCall)]
        public static extern uint QueryDosDevice(string lpDeviceName, StringBuilder lpTargetPath, int ucchMax);

        [DllImport("kernel32.dll", EntryPoint = "FindFirstVolumeMountPoint", SetLastError = true, CallingConvention = CallingConvention.StdCall)]
        public static extern uint FindFirstVolumeMountPoint(string lpszRootPathName, StringBuilder lpszVolumeMountPoint, int ucchMax);
    }
}
