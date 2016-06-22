: # This is a special script which intermixes both sh
: # and cmd code. It is written this way because it is
: # used in system() shell-outs directly in otherwise
: # portable code. See http://stackoverflow.com/questions/17510688
: # for details.
:; mkdir -p $1; exit
@ECHO OFF
IF NOT EXIST %1 (
mkdir %1 > nul 2> nul
)