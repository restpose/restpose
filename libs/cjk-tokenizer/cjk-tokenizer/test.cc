#include <iostream>
#include "cjk-tokenizer.h"

using namespace std;
using namespace cjk;

class handler : public tokenizer_handler {
public:
    void handle_token(const std::string &tok, bool is_cjk) {
        cout << "Handling [" << tok << "] by handler class. "
             << "(CJK: " << is_cjk << ")" << endl;
    };
};

void test1(string text_str) {
    tokenizer tknzr;
    vector<pair<string, unsigned> > token_list;
    vector<pair<string, unsigned> >::iterator token_iter;
    cout << "[Default]" << endl;
    token_list.clear();
    cout << "Ngram size: "  << tknzr.ngram_size << endl;
    tknzr.tokenize(text_str, token_list);
    cout << "Original string: " << text_str << endl;
    cout << "Tokenized result: ";
    for (token_iter = token_list.begin();
         token_iter != token_list.end(); ++token_iter) {
        cout << "[" << token_iter->first << "," << token_iter->second << "] ";
    }
    cout << endl << endl;

    cout << "[Trigram]" << endl;
    token_list.clear();
    tknzr.ngram_size = 3;
    cout << "Ngram size: "  << tknzr.ngram_size << endl;
    tknzr.tokenize(text_str, token_list);
    cout << "Original string: " << text_str << endl;
    cout << "Tokenized result: ";
    for (token_iter = token_list.begin();
         token_iter != token_list.end(); ++token_iter) {
        cout << "[" << token_iter->first << "] ";
    }
    cout << endl << endl;

    cout << "[Pentagram]" << endl;
    token_list.clear();
    tknzr.ngram_size = 5;
    cout << "Ngram size: "  << tknzr.ngram_size << endl;
    tknzr.tokenize(text_str, token_list);
    cout << "Original string: " << text_str << endl;
    cout << "Tokenized result: ";
    for (token_iter = token_list.begin();
         token_iter != token_list.end(); ++token_iter) {
        cout << "[" << token_iter->first << "] ";
    }
    cout << endl << endl;

    cout << "[Max token count]" << endl;
    token_list.clear();
    tknzr.max_token_count = 10;
    cout << "Max token count: " << tknzr.max_token_count << endl;
    tknzr.tokenize(text_str, token_list);
    cout << "Original string: " << text_str << endl;
    cout << "Tokenized result: ";
    for (token_iter = token_list.begin();
         token_iter != token_list.end(); ++token_iter) {
        cout << "[" << token_iter->first << "] ";
    }
    cout << endl << endl;

    cout << "[Unigram]" << endl;
    token_list.clear();
    tknzr.ngram_size = 1;
    tknzr.max_token_count = 0;
    tknzr.tokenize(text_str, token_list);
    cout << "Original string: " << text_str << endl;
    cout << "Tokenized result: ";
    for (token_iter = token_list.begin();
         token_iter != token_list.end(); ++token_iter) {
        cout << "[" << token_iter->first << "] ";
    }
    cout << endl << endl;
}

void test2(string text_str) {
    tokenizer tknzr;
    vector<string> token_list;
    vector<string>::iterator token_iter;
    cout << "[Split]" << endl;
    token_list.clear();
    tknzr.split(text_str, token_list);
    cout << "Original string: " << text_str << endl;
    cout << "Tokenized result: ";
    for (token_iter = token_list.begin();
         token_iter != token_list.end(); ++token_iter) {
        cout << "[" << *token_iter << "] ";
    }
    cout << endl << endl;

    vector<unicode_char_t> uchar_list;
    vector<unicode_char_t>::iterator uchar_iter;

    cout << "[Split (unicode_char_t)]" << endl;
    tknzr.split(text_str, uchar_list);
    cout << "Original string: " << text_str << endl;
    cout << "Tokenized result: ";
    for (uchar_iter = uchar_list.begin();
         uchar_iter != uchar_list.end(); uchar_iter++) {
        cout << "[" << *uchar_iter << "] ";
    }
    cout << endl << endl;

    cout << "-- CJK Segmentation" << endl;
    cout << "Original string: " << text_str << endl;
    token_list.clear();
    tknzr.segment(text_str, token_list);
    cout << "Tokenized result: ";
    for (token_iter = token_list.begin();
         token_iter != token_list.end(); ++token_iter) {
        cout << "[" << *token_iter << "] ";
    }
    cout << endl << endl;

    cout << "[Split] (zh_tw -> zh_cn)" << endl;
    token_list.clear();
    tknzr.han_conv_method = HAN_CONV_TRAD2SIMP;
    tknzr.split(text_str, token_list);
    cout << "Original string: " << text_str << endl;
    cout << "Tokenized result: ";
    for (token_iter = token_list.begin();
         token_iter != token_list.end(); ++token_iter) {
        cout << "[" << *token_iter << "] ";
    }
    cout << endl << endl;

    cout << "[Split] (zh_cn -> zh_tw)" << endl;
    token_list.clear();
    tknzr.han_conv_method = HAN_CONV_SIMP2TRAD;
    tknzr.split(text_str, token_list);
    cout << "Original string: " << text_str << endl;
    cout << "Tokenized result: ";
    for (token_iter = token_list.begin();
         token_iter != token_list.end(); ++token_iter) {
        cout << "[" << *token_iter << "] ";
    }
    cout << endl << endl;

    handler h;
    cout << "-- CJK Tokenizer Handler" << endl;
    tknzr.han_conv_method = HAN_CONV_NONE;
    tknzr.tokenize(text_str, h);
    cout << endl << endl;
    
    string cjk_str = "這是CJK字串";
    string pure_cjk_str =
        "這個字串只含中日韓。"
        "コンピューターの機能を、音響・映像・作品制御などに利用する芸術の総称。"
        "납치 여중생 공포에 떠는데'…경찰 200m 거리 25분만에 출동"
        "ㄅㄆㄇㄈㄉㄊㄋㄌㄧㄨㄩ";
    cout << "[" << cjk_str << "]" << " has CJK characters? "
         << tknzr.has_cjk(cjk_str) << endl;
    cout << "[" << cjk_str << "]" << " has CJK characters only? "
         << tknzr.has_cjk_only(cjk_str) << endl;
    cout << "[" << pure_cjk_str << "]" << " has CJK characters? "
         << tknzr.has_cjk(pure_cjk_str) << endl;
    cout << "[" << pure_cjk_str << "]" << " has CJK characters only? "
         << tknzr.has_cjk_only(pure_cjk_str) << endl;
    cout << endl;
}

int main() {
    char text[] =
        "美女遊戲等你挑戰周蕙最新鈴搶先下載茄子醬耍可愛一流"
        "华沙是波兰的首都，也是其最大的城市。"
        "납치 여중생 공포에 떠는데'…경찰 200m 거리 25분만에 출동"
        "寛永通宝の一。京都方広寺の大仏をこわして1668年（寛文8）から鋳造した銅銭。"
        "ㄅㄆㄇㄈㄉㄊㄋㄌㄧㄨㄩ"
        "Giant Microwave Turns Plastic Back to Oil";
    string text_str = string(text);

    test1(text_str);
    test2(text_str);
    return 0;
}
