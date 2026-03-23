// when running this at the command line, 
// call it with CScript so you don't get UI. 
// example: CScript convert.js e:\yourprojects\old.dsp e:\yourprojects\new.vcproj
// NOTE: full path required to both input and output files

// or set default script engine to the command line doing this first
// example: CScript //H:CScript

// Once you set the environment, run the .js file like a .bat file

// To have a batch file loop through all the .dsp files 
// in a directory, write a batch file that looked like this
// (Windows NT 4 or Windows 2000 only)
// CScript //H:CScript //Nologo
// for /R %%i in (*.dsp) do convert.js %%i >> .\Convert.log

var vcProj = new ActiveXObject("VisualStudio.VCProjectEngine.7.1");
var objFile = new ActiveXObject("Scripting.FileSystemObject");
var objArgs = WScript.Arguments;

// check the arguments to be sure it's right
if (objArgs.Count() < 2)
{
   WScript.Echo("VC6 or 5 DSP Project File Conversion");
   WScript.Echo("Opens specified .dsp and converts to VC7.1 Format.");
   WScript.Echo("Will create project file with .vcproj extension");
   WScript.Echo("\n\tusage: <full path\project.dsp> <full path\project.vcproj>");
   WScript.Quit(1);
}

WScript.Echo("\nConverting: "+ objArgs.Item(0));
// If there is a file name of the .vcproj extension, do not convert
var vcProject = vcProj.LoadProject(objArgs.Item(0));
if (!objFile.FileExists(vcProject.ProjectFile))
{
   // specify name and location of new project file
   vcProject.ProjectFile = objArgs.Item(1);

   // call the project engine to save this off. 
   // when no name is shown, it will create one with the .vcproj name
   vcProject.Save();
   WScript.Echo("New Project Name: "+vcProject.ProjectFile+"\n");
}
else
{
   WScript.Echo("ERROR!: "+vcProject.ProjectFile+" already exists!\n");
}
