#ifndef __CJK_HAN_CONVERT_H__
#define __CJK_HAN_CONVERT_H__

#include <vector>
#include <xapian.h>

typedef unsigned unicode_char_t;

namespace cjk {
    class han_convert {
    public:
        static void simp2trad(unicode_char_t &uchar);
        static void trad2simp(unicode_char_t &uchar);

        static void simp2trad(std::vector<unicode_char_t> &uvector);
        static void trad2simp(std::vector<unicode_char_t> &uvector);
    };
}

#endif
