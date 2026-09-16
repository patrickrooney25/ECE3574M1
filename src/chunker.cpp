#include "aiws/chunker.hpp"
#include "aiws/text_processor.hpp"

#include <stdexcept>
#include <algorithm>


namespace aiws {

Chunker::Chunker(ChunkingPolicy policy) : policy_(policy) {
    if (policy_.max_tokens == 0 || policy_.overlap >= policy_.max_tokens ||
        policy_.paragraph_window > policy_.max_tokens) {
        throw std::invalid_argument("invalid chunking policy");
    }
}

std::vector<Chunk> Chunker::chunk(const Document& document, std::size_t document_order) const {
    // TODO: produce deterministic, source-attributed chunks for the supplied document.
    std::vector<Chunk> chunks;
    std::vector<TokenInfo> tokens = TextProcessor::tokenize(document.text());

    if (tokens.empty()){ //empty doc
        return chunks;
    }

    std::size_t start_token_idx=0;
    std::size_t chunk_sequence=0;
    const std::size_t total_tokens =tokens.size();

    while(start_token_idx<total_tokens){
        std::size_t remaining_tokens = total_tokens - start_token_idx;
        std::size_t end_token_idx = start_token_idx;

        if(remaining_tokens<=policy_.max_tokens){
            end_token_idx=total_tokens; //final short chunk takes all tokens left
        }
        else{
            std::size_t max_end_idx= start_token_idx +policy_.max_tokens; //max cutoff
            std::size_t window_start_idx = max_end_idx - policy_.paragraph_window; //preference window

            std::size_t paragraph_split_idx=0; //search for latest paragraph boundary in window
            bool found_paragraph_boundary=false;

            for (std::size_t i = max_end_idx; i > window_start_idx; i--){
                if (i>start_token_idx && tokens[i-1].paragraph< tokens[i].paragraph){ //tokens index exceeds previous tokens, so boundary is created
                    paragraph_split_idx=i;
                    found_paragraph_boundary=true;
                    break;
                }
            }
            if(found_paragraph_boundary){
                end_token_idx=paragraph_split_idx;
            }
            else{
                end_token_idx=max_end_idx;
            }
        }

        std::string chunk_id=document.id() + "#" + std::to_string(chunk_sequence);
        std::string normalized_text = TextProcessor::join(tokens, start_token_idx, end_token_idx);
        std::size_t token_count = end_token_idx - start_token_idx;

        std::size_t source_begin = tokens[start_token_idx].begin;
        std::size_t source_end = tokens[end_token_idx-1].end;

        chunks.push_back({chunk_id, document.id(), chunk_sequence, document_order, normalized_text, token_count, source_begin, source_end});

        if (end_token_idx >= total_tokens){
            break;
        }

        start_token_idx =end_token_idx- policy_.overlap;
        chunk_sequence++;
    }
        return chunks;
}

}  // namespace aiws
