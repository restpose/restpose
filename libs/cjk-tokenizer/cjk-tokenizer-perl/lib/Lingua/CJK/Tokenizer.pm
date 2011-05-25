package Lingua::CJK::Tokenizer;

use strict;
use XSLoader;

XSLoader::load 'Lingua::CJK::Tokenizer';

1;
__END__

=pod

=head1 NAME

Lingua::CJK::Tokenizer - CJK Tokenizer

=head1 SYNOPSIS

    my $tknzr = Lingua::CJK::Tokenizer->new();
    $tknzr->ngram_size(5);
    $tknzr->max_token_count(100);
    $tokens_ref = $tknzr->tokenize("CJK Text");
    $tokens_ref = $tknzr->segment("CJK Text");
    $tokens_ref = $tknzr->split("CJK Text");
    $flag = $tknzr->has_cjk("CJK Text");
    $flag = $tknzr->has_cjk_only("CJK Text");

=head1 DESCRIPTION

This module tokenizes CJK texts into n-grams.

=head1 METHODS

=head2 ngram_size

sets the size of returned n-grams

=head2 max_token_count

sets the limit on the number of returned n-grams in case input text is too long or of indefinite size

=head2 tokenize

tokenizes texts into n-grams

=head2 segment

cuts cjk texts into chunks

=head2 split

tokenizes texts into uni-grams.

=head2 has_cjk

returns true if text has cjk characters

=head2 has_cjk_only

returns true if text has only cjk characters

=head1 PREREQUISITE

This module requires libunicode by Tom Tromey.

=head1 COPYRIGHT

Copyright (c) 2009 Yung-chung Lin. 

This program is free software; you can redistribute it and/or modify
it under the MIT License.

=cut
