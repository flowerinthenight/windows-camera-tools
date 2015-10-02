Edit:
  You can use ecmangen.exe

Compile:
  VS command prompt> mc -um <name>.man

Include:
  1. Include trace.h header to use the trace functions.
  2. Add trace.rc file to a component (dll, exe) as resource.

Register manifest:
  wevtutil.exe im trace.man /rf:"fullpath_comp_containing_rc_file" /mf:"fullpath_comp_containing_rc_file"