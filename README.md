# multi-tail Tail multiple files using tokens

I could not find a tool that would tail multiple files easily from the
Linux command line.  I borrowed some of the internals from tailf and
added threads and the ability to match strings in each file being
tailed.


usage: mtail -f <path-to-file-1>:<pattern> -f <path-to-file-2>:<pattern> ...


example:
/mtail -f /var/log/syslog:error -f /var/log/dmesg

Returns all lines in syslog matching string "error" and any new lines in dmesg.
