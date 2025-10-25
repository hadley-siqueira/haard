#ifndef HAARD_TOKEN_H
#define HAARD_TOKEN_H

namespace haard {
    class Token {
        public:
            void set_offset(unsigned int offset);
            void set_line(unsigned int line);
            void set_column(unsigned int column);
            void set_whitespace(unsigned int whitespace);
            void set_value(char* value);

            unsigned int get_offset();
            unsigned int get_line();
            unsigned int get_column();
            unsigned int get_whitespace();
            char* get_value();

        private:
            unsigned int whitespace;
            unsigned int offset;
            unsigned int line;
            unsigned int column;
            char* value;
    };
}

#endif
