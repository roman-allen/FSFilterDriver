using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace FSFilterDriverConsole
{
    class Program
    {
        private static FSFilterDriverAPI.FSFilterDriverAPI api;
        delegate void CustomLogic();

        static void Main(string[] args)
        {
            if (args.Length == 1 && args[0] == "/monitor")
            {
                StartMonitor();
                return;
            }

            if (args.Length == 2 && args[0] == "/lock")
            {
                LockFile(args[1]);
                return;
            }

            if (args.Length == 2 && args[0] == "/unlock")
            {
                UnlockFile(args[1]);
                return;
            }

            if (args.Length == 2 && args[0] == "/allow")
            {
                AllowProcess(args[1]);
                return;
            }

            if (args.Length == 2 && args[0] == "/deny")
            {
                DenyProcess(args[1]);
                return;
            }

            Console.WriteLine("USAGE:");
            Console.WriteLine();
            Console.WriteLine("FSFilterDriverConsole /lock < full_file_path >");
            Console.WriteLine("     /lock – Add file to list of locked files and notifies driver that list is updated");
            Console.WriteLine();
            Console.WriteLine("FSFilterDriverConsole /unlock < full_file_path >");
            Console.WriteLine("     /unlock – Removes file from list of locked files and notifies driver that list is updated");
            Console.WriteLine();
            Console.WriteLine("FSFilterDriverConsole /allow < process_name >");
            Console.WriteLine("     /allow – Adds process to white list of processes which allowed to change locked files");
            Console.WriteLine();
            Console.WriteLine("FSFilterDriverConsole /deny < process_name >");
            Console.WriteLine("     /deny – Removes process from white list of processes which allowed to change locked files");
            Console.WriteLine();
            Console.WriteLine("FSFilterDriverConsole /monitor");
            Console.WriteLine("     /monitor – Makes application to wait for notifications from driver.This command will print names of files which are lock and have been accessed by OS for write / move / delete actions.");
        }

        private static void DenyProcess(string v)
        {
            RunAPI(delegate() {
                api.SendProcessesWhiteList(new List<string>());
            });
        }

        private static void AllowProcess(string v)
        {
            RunAPI(delegate () {
                List<string> list = new List<string>();
                list.Add(v);
                api.SendProcessesWhiteList(list);
            });
        }

        private static void UnlockFile(string v)
        {
            RunAPI(delegate () {
                List<string> list = new List<string>();
                list.Add(v);
                api.UnlockFiles(list, true);
            });
        }

        private static void LockFile(string v)
        {
            RunAPI(delegate () {
                List<string> list = new List<string>();
                list.Add(v);
                api.LockFiles(list, true);
            });
        }

        private static void StartMonitor()
        {
            RunAPI(delegate () {
                Console.WriteLine("Waiting for lock notifications...");
                while (true)
                {
                    var list = api.GetReportedItems().Distinct();
                    foreach (var str in list) Console.WriteLine(str);
                    Thread.Sleep(1000);
                }
            });
        }

        private static void RunAPI(CustomLogic logic)
        {
            Console.WriteLine("Create API instance...");
            api = new FSFilterDriverAPI.FSFilterDriverAPI();
            try
            {
                Console.WriteLine("Attempt to connect...");
                api.Connect();
                Console.WriteLine("Connected to driver sucessfully");

                logic();

                Console.WriteLine("Disconnecting...");
                api.Disconnect();
                Console.WriteLine("Disconnected from driver sucessfully");
            }
            catch (Exception ex)
            {
                Console.WriteLine("EXCEPTION: " + ex.Message);
                Console.ReadLine();
            }
            finally
            {
                try
                {
                    if (api.IsConnected)
                    {
                        api.Disconnect();
                    }
                }
                catch { };
            }

        }
    }
}
