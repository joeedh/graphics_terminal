The goal of this project is to design a remote drawing API
that's light enough to run microcontroller front-end GUIs
off serial lines.

This includes a small shader language (well, bytecode)
that's desiged to be GPU-compatible (though right now
the bytecode is compiled to x86 SSE instructions).

It would be nice if SVG had a binary format, but of course,
the Unix hippies could never stand for that. . .
