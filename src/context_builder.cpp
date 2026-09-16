#include "aiws/context_builder.hpp"
#include "aiws/text_processor.hpp"

#include <unordered_set>
#include <algorithm>

namespace aiws {

std::vector<ContextItem> ContextBuilder::build(const std::vector<SearchResult>& ranked, std::size_t token_budget) const {    
    if (token_budget==0 || ranked.empty()){
        return {};
    }

    std::vector<ContextItem> context_items;
    std::unordered_set<std::string> included_chunk_ids;
    std::size_t used_budget=0;

    for(std::size_t i=0; i<ranked.size(); i++){
        const SearchResult& result =ranked[i];

        if(included_chunk_ids.find(result.chunk_id)!= included_chunk_ids.end()){ //deduplicated chunks by id
            continue;
        }

        std::size_t remaining_budget =token_budget -used_budget;
        if(remaining_budget==0){
            break;
        }

        //tokenize search results text
        std::vector<std::string> terms = TextProcessor::terms(result.text);
        std::size_t chunk_token_count =terms.size();

        if (chunk_token_count <= remaining_budget){
            ContextItem item{}; //whole chunk fits in budget
            item.chunk_id =result.chunk_id;
            item.document_id =result.document_id;
            item.chunk_sequence =result.chunk_sequence;
            item.text = result.text;
            item.token_count =chunk_token_count;
            item.score= result.score;
            item.truncated = false;

            context_items.push_back(item);
            included_chunk_ids.insert(result.chunk_id);
            used_budget+=chunk_token_count;
        }
        else{
            std::string truncated_text = TextProcessor::join(terms, 0, remaining_budget); //chunk exceeds remaining budget, must truncate

            ContextItem item{};
            item.chunk_id = result.chunk_id;
            item.chunk_sequence = result.chunk_sequence;
            item.text=truncated_text;
            item.token_count = remaining_budget;
            item.score =result.score;
            item.truncated=true;

            context_items.push_back(item);
            included_chunk_ids.insert(result.chunk_id);
            used_budget+=remaining_budget;
            break;
        }
    }

    return context_items;
}

}  // namespace aiws
