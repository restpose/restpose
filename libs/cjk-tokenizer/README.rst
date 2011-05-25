Description
===========

This module is a word tokenizer for CJK texts, supporting n-gram tokenization.
It is designed to be used with Xapian (http://xapian.org), and uses Xapian's
unicode routines.

Currently, there is no documentation other than the source code.

Authors
=======

 - Original author: Yung-chung Lin (henearkrxern@gmail.com)
 - Unicode modifications: Richard Boulton (richard@tartarus.org)

Features
========

 - N-gram tokenization on CJK texts.
 - Conversion from Traditional Chinese to Simplified Chinese, and vice versa.

History
=======

This project was taken from http://code.google.com/p/cjk-tokenizer/ , but then
modified to use Xapian's internal unicode routines.
