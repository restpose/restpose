use Test::More tests => 8;
use strict;
use Lingua::CJK::Tokenizer;

my $tknzr = Lingua::CJK::Tokenizer->new();

my $text_str = "美女遊戲等你挑戰周蕙最新鈴搶先下載茄子醬耍可愛一流"
    . "납치 여중생 공포에 떠는데'…경찰 200m 거리 25분만에 출동"
    . "寛永通宝の一。京都方広寺の大仏をこわして1668年（寛文8）から鋳造した銅銭。"
    . "ㄅㄆㄇㄈㄉㄊㄋㄌㄧㄨㄩ"
    . "Giant Microwave Turns Plastic Back to Oil";

my $tokens_ref;

$tokens_ref = $tknzr->tokenize($text_str);
is(scalar(@$tokens_ref), 174);

$tknzr->ngram_size(5);
$tokens_ref = $tknzr->tokenize($text_str);
is(scalar(@$tokens_ref), 391);

$tknzr->ngram_size(3);
$tknzr->max_token_count(5);
$tokens_ref = $tknzr->tokenize($text_str);
is(scalar(@$tokens_ref), 5);

$tokens_ref = $tknzr->split($text_str);
is(scalar(@$tokens_ref), 151);

$tokens_ref = $tknzr->segment($text_str);
is($tokens_ref->[0], '美女遊戲等你挑戰周蕙最新鈴搶先下載茄子醬耍可愛一流납치');
is(scalar(@$tokens_ref), 14);

ok($tknzr->has_cjk($text_str));
ok(!$tknzr->has_cjk_only($text_str));
