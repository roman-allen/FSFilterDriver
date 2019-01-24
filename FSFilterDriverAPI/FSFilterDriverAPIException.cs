using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace FSFilterDriverAPI
{
    public class FSFilterDriverAPIException: Exception
    {
        public FSFilterDriverAPIException(): base()
        {
        }

        public FSFilterDriverAPIException(string message): base(message)
        {
        }
    }
}
