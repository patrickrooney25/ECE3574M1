#include "aiws/text_processor.hpp"
#include <cctype>
#include <algorithm>

namespace aiws {

    bool is_token_char(char c) {
        return std::isalnum(static_cast<unsigned char>(c)) !=0;
    }

    char normalize_char(char c) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

std::vector<TokenInfo> TextProcessor::tokenize(const std::string& text) {
    // TODO: produce normalized tokens with source and paragraph information.
    std::vector<TokenInfo> tokens;
    std::size_t i=0;
    const std::size_t n = text.size();

    std::size_t current_par=0;
    std::size_t line_start=0;
    bool saw_new_line=false;

    while (i<n){
        char c=text[i];

        if (c=='\n'){ //track paragraph boundaries
            if(saw_new_line){
                bool blank_line=true;
                for (std::size_t j=line_start; j<i; j++){ // check space or tab
                    if(text[j] != ' ' && text[j] != '\t' && text[j] != '\r'){
                        blank_line=false;
                        break;
                    }
                }
                if(blank_line){
                    current_par++;
                }
            }
            saw_new_line=true;
            line_start=i+1;
            i++;
            continue;
        }

        if (is_token_char(c)){ //non new line char
            std::size_t token_start=i;
            std::string normalized_token;

            while(i<n && is_token_char(text[i])){
                normalized_token += normalize_char(text[i]);
                i++;
            }

            tokens.push_back({normalized_token, token_start, i, current_par});
        }
        else {
            i++;
        }
    }

    return tokens;
}

std::vector<std::string> TextProcessor::terms(const std::string& text) {
    // TODO: return the normalized terms represented by the input text.
    std::vector<TokenInfo> token_infos = tokenize(text);
    std::vector<std::string> result;
    result.reserve(token_infos.size());

    for (std::size_t i = 0; i < token_infos.size(); i++) {
        result.push_back(token_infos[i].token);
    }

    return result;
}

std::string TextProcessor::normalize(const std::string& text) {
    // TODO: return the normalized form of the input text.
    std::vector<std::string> term_list =terms(text);
    return join(term_list, 0, term_list.size());
}

std::string TextProcessor::join(const std::vector<TokenInfo>& tokens,
                                std::size_t begin,
                                std::size_t end) {
    // TODO: join the requested token range into normalized text.
    if (begin>=end || begin >=tokens.size()){
        return "";
    }

    std::size_t actual_end = std::min(end, tokens.size());
    std::string result =tokens[begin].token;

    for (std::size_t i=begin+1; i<actual_end; i++){
        result += ' ';
        result +=tokens[i].token;
    }

    return result;
}

std::string TextProcessor::join(const std::vector<std::string>& tokens,
                                std::size_t begin,
                                std::size_t end) {
    // TODO: join the requested term range into normalized text.
    if (begin>=end || begin >=tokens.size()){
        return "";
    }

    std::size_t actual_end = std::min(end, tokens.size());
    std::string result =tokens[begin];

    for (std::size_t i=begin+1; i<actual_end; i++){
        result += ' ';
        result +=tokens[i];
    }

    return result;
}

}  // namespace aiws
