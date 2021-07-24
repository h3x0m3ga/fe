# fe
Frontend is a Webkit project to combine web technologies with the shell. Make front ends for command line programs. Use HTML/CSS/JAVASCRIPT to build GUIs. Very lightweight, versatile.

Flags:
  -d            debug, start the Webkit web inspector with fe.
  -i            inhibit, do not execute commands.
  -v            verbose, produce copious output.
  -f %filename% file, read from file instead of stdin.

Usage:
  fe can read from STDIN(default) or from a file (with -f flag).
  
  STDIN
    fe < index.html
  
  STDIN (HEREDOCUMENT)  
    fe << COMPLETE
      /* todo
        fill in small example
      */
      
    COMPLETE
    
  File
    fe -f index.html
  
  