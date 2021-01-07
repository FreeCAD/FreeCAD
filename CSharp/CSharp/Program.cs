using System;
using System.Runtime.InteropServices;

namespace FreeCADNET
{
    class Program
    {
        [DllImport("FreeCADBase.dll")]
        public static extern void EmptyMethod();
        [DllImport("FreeCADApp.dll")]
        public static extern void EmptyMethod2();

        public static void Main(string[] args)
        {
            const string sBanner = "(C) BIG NOSES INC. (2020-2021)";

            // Make sure that we use '.' as decimal point [NOT IMPLEMENTED YET]
            // setlocale(LC_ALL, ""); // default C++ method
            // setlocale(LC_NUMERIC, "C"); // same

            // Name and version of the application [NOT IMPLEMENTED YET]
            // App::Application::Config()["ExeName"] = "FreeCAD"; // fromFreeCADApp
            // App::Application::Config()["ExeVendor"] = "FreeCAD";
            // App::Application::Config()["AppDataSkipVendor"] = "true";

            // Set the banner (for logging and console) [NOT IMPLEMENTED YET]
            // App::Application::Config()["CopyrightInfo"] = sBanner;

            // Init phase ======================================================================================
            try
            {
                // Sets the default run code for FC, starts with command prompt if not overridden in InitConfig.
                // App::Application::Config()["RunMode"] = "Exit";
                // App::Application::Config()["LoggingConsole"] = "1";

                // Inits the Application.
                // App::Application::init(args.Length,args);
            }
            /*catch (Exception e)
            {
                Console.Write(e.what());    
                System.Environment.Exit(1);
            }
            catch (Exception e)
            {
                Console.Write(e.what());
                System.Environment.Exit(0);
            }
            catch (Exception e)
            {
                string appName = App:Application::Config()["ExeName"];
                string msg = "";
                msg += "While initializing " + appName + " the following exception occurred: \'" + e.what() + "\'\n\n";
                msg += "Python is searching for its runtime files in the following directories:\n" + Py_GetPath() + "\n\n";
                msg += "Python version information:\n" + Py_GetVersion() + "\n";
                string pythonhome = getenv("PYTHONHOME");
                if (pythonhome)
                {
                    msg += "\nThe environment variable PYTHONHOME is set to \'" + pythonhome + "\'.";
                    msg += "\nSetting this environment variable may cause Python to fail. Please contact your environment to unset it on your system.\n\n";
                }
                else
                {
                    msg += "\nPlease contact the application's support team for more information.\n\n";
                }
                Console.Write("Initialization of " + appName + " failed:\n" + msg);
                System.Environment.Exit(100);
            }*/
            catch (Exception e)
            {
                string appName = "FreeCAD?"; // should call App::Application::Config()["ExeName"]
                string msg = "";
                msg += "Unknown runtime error occurred while initializing " + appName + ".\n\n";
                msg += "Please contact the application's support team for more information.\n\n";
                Console.Write("Initialization of " + appName + " failed:\n" + msg);
                System.Environment.Exit(101);
            }

            // Run phase. ======================================================================================
            try
            {
                // Application::runApplication();
            }
            /*catch (Exception e)
            {
                System.Environment.Exit(e.getExitCode());
            }
            catch (Exception e)
            {
                // e.ReportException();
                System.Environment.Exit(1);
            }*/
            catch (Exception e)
            {
                Console.Write("Application unexpectedly terminated.\n");
                System.Environment.Exit(1);
            }

            // Destruction phase ======================================================================================
            Console.Write("Application terminating...\n");

            try
            {
                // App:GetApplication().closeAllDocuments();
            }
            catch (Exception e) { }

            // Application::destruct();
            Console.Write("FreeCAD completely terminated.\n");

        }
    }
}
