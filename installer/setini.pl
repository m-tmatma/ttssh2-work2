#!/usr/bin/perl

open INI, $ARGV[0];
while(<INI>){
	s/^(Language\s*=).*$/$1Japanese/;
#	s/^(VTFont\s*=).*$/$1Terminal,0,-19,128/;
	s/^(VTFont\s*=).*$/$1�l�r �S�V�b�N,0,-16,128/;
	s/^(TEKFont\s*=).*$/$1Terminal,0,8,128/;
	s/^(TCPPort\s*=).*$/${1}22/;
	print;
}
close INI;
