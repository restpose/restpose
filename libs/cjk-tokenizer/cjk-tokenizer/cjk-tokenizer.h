#ifndef __CJK_TOKENIZER_H__
#define __CJK_TOKENIZER_H__

#include <string>
#include <vector>
#include <xapian.h>

typedef unsigned unicode_char_t;

namespace cjk {
    class tokenizer_handler {
    public:
        virtual void handle_token(const std::string &, bool) {}
        virtual ~tokenizer_handler() {}
    };

    enum han_conv_enum {
        HAN_CONV_NONE = 0,
        HAN_CONV_TRAD2SIMP,
        HAN_CONV_SIMP2TRAD
    };
    
    class tokenizer {
    private:
        static inline unsigned char* _unicode_to_char(unicode_char_t &uchar,
                                                      unsigned char *p);
    public:
        han_conv_enum han_conv_method;
        unsigned int ngram_size;
        unsigned int max_token_count;

        tokenizer();
        ~tokenizer();
        void tokenize(const std::string &str,
                      std::vector<std::pair<std::string, unsigned int> > &token_list);
        void tokenize(const std::string &str,
                      tokenizer_handler &handler);

        void split(const std::string &str,
                   std::vector<std::string> &token_list);
        void split(const std::string &str,
                   std::vector<unicode_char_t> &token_list);

        void segment(std::string str,
                     std::vector<std::string> &token_segment);

        bool has_cjk(const std::string &str);
        bool has_cjk_only(const std::string &str);
    };
};

#endif
