use strict;
use ExtUtils::MakeMaker;

my $CC = $ENV{"CXX"} || 'g++';
my $LD = '$(CC)';
my $INC = '-Icjk-tokenizer';

system("cd cjk-tokenizer; make");

WriteMakefile(NAME          => 'Lingua::CJK::Tokenizer',
	      VERSION       => '0.01',
	      PREREQ_PM     => {},
	      ABSTRACT      => 'CJK Tokenizer',
	      AUTHOR        => 'Yung-chung Lin <henearkrxern@gmail.com>',
	      LIBS          => '-Lcjk-tokenizer -lcjk-tokenizer -lunicode',
	      CC            => $CC,
	      LD            => $LD,
	      INC           => $INC,
	      XSOPT         => '-C++',
	      TYPEMAPS      => ['typemap']);
