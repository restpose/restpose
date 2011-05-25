#ifdef __cplusplus
extern "C" {
#endif
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#ifdef __cplusplus
}
#endif

#include "cjk-tokenizer.h"

using namespace std;
using namespace cjk;

MODULE = Lingua::CJK::Tokenizer		PACKAGE = Lingua::CJK::Tokenizer

PROTOTYPES: ENABLE

tokenizer * new(SV* CLASS)
    CODE:
        RETVAL = new tokenizer();
    OUTPUT:
        RETVAL

void
tokenizer::ngram_size(U32 ngram_size)
    CODE:
        THIS->ngram_size = ngram_size;

void
tokenizer::max_token_count(U32 max_token_count)
    CODE:
        THIS->max_token_count = max_token_count;

SV*
tokenizer::tokenize(SV* str)
    CODE:
        if (!SvPOK(str)
            || !is_utf8_string((U8*)SvPV(str, SvCUR(str)), SvCUR(str))) {
	    Perl_croak(aTHX_ "The input must be a UTF-8 string");
            XSRETURN_UNDEF;
        }
        vector<string> token_list;
        vector<string>::iterator token_iter;
        string s((const char*) SvPV(str, SvCUR(str)));
        THIS->tokenize(s, token_list);

        AV* tokens = newAV();
        for (token_iter = token_list.begin(); token_iter != token_list.end();
             token_iter++) {
            av_push(tokens, newSVpv(token_iter->c_str(),
                                    (STRLEN) token_iter->length()));
        }
        RETVAL = newRV_noinc((SV*) tokens);
    OUTPUT:
        RETVAL

SV*
tokenizer::split(SV* str)
    CODE:
        if (!SvPOK(str)
            || !is_utf8_string((U8*)SvPV(str, SvCUR(str)), SvCUR(str))) {
	    Perl_croak(aTHX_ "The input must be a UTF-8 string");
            XSRETURN_UNDEF;
        }
        vector<string> token_list;
        vector<string>::iterator token_iter;
        string s((const char*) SvPV(str, SvCUR(str)));
        THIS->split(s, token_list);

        AV* tokens = newAV();
        for (token_iter = token_list.begin(); token_iter != token_list.end();
             token_iter++) {
            av_push(tokens, newSVpv(token_iter->c_str(),
                                    (STRLEN) token_iter->length()));
        }
        RETVAL = newRV_noinc((SV*) tokens);
    OUTPUT:
        RETVAL

SV*
tokenizer::segment(SV* str)
    CODE:
        if (!SvPOK(str)
            || !is_utf8_string((U8*)SvPV(str, SvCUR(str)), SvCUR(str))) {
	    Perl_croak(aTHX_ "The input must be a UTF-8 string");
            XSRETURN_UNDEF;
        }
        vector<string> token_list;
        vector<string>::iterator token_iter;
        string s((const char*) SvPV(str, SvCUR(str)));
        THIS->segment(s, token_list);

        AV* tokens = newAV();
        for (token_iter = token_list.begin(); token_iter != token_list.end();
             token_iter++) {
            av_push(tokens, newSVpv(token_iter->c_str(),
                                    (STRLEN) token_iter->length()));
        }
        RETVAL = newRV_noinc((SV*) tokens);
    OUTPUT:
        RETVAL

bool
tokenizer::has_cjk(SV* str)
    CODE:
        if (!SvPOK(str)
            || !is_utf8_string((U8*)SvPV(str, SvCUR(str)), SvCUR(str))) {
	    Perl_croak(aTHX_ "The input must be a UTF-8 string");
            XSRETURN_UNDEF;
        }
        string s((const char*) SvPV(str, SvCUR(str)));
        RETVAL = THIS->has_cjk(s);
    OUTPUT:
        RETVAL

bool
tokenizer::has_cjk_only(SV* str)
    CODE:
        if (!SvPOK(str)
            || !is_utf8_string((U8*)SvPV(str, SvCUR(str)), SvCUR(str))) {
	    Perl_croak(aTHX_ "The input must be a UTF-8 string");
            XSRETURN_UNDEF;
        }
        string s((const char*) SvPV(str, SvCUR(str)));
        RETVAL = THIS->has_cjk_only(s);

    OUTPUT:
        RETVAL

void
tokenizer::DESTROY()
