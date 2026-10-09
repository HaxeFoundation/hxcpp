// The null device is a character device, so _isatty reports it as a tty. file_write used that as
// its test for a console and called WriteConsoleW, which only accepts a console handle; the failure
// threw from inside a GC-free zone and took the process down instead of reporting an error.
function main() {
	final nul = Sys.systemName() == "Windows" ? "NUL" : "/dev/null";

	final out = sys.io.File.write(nul);
	out.writeString("written to the null device\n");
	out.flush();
	out.close();

	Sys.println("ok");
}
