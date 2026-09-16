#include "aiws/corpus_index.hpp"
#include "aiws/text_processor.hpp"

#include <stdexcept>
#include <sstream>

namespace aiws {

CorpusIndex::CorpusIndex(const std::vector<Chunk>& chunks) {
    build(chunks);
}

void CorpusIndex::build(const std::vector<Chunk>& chunks) {
    // TODO: build the searchable index from the supplied chunks.
    postings_.clear();
    chunk_by_id_.clear();

    for (std::size_t i=0; i<chunks.size(); i++){
        const auto& chunk = chunks[i];
        chunk_by_id_[chunk.id]=i;

        std::vector<std::string> terms = TextProcessor::terms(chunk.text); //tokenize chunks text
        std::unordered_map<std::string, std::size_t> term_counts;

        for (std::size_t j=0; j<terms.size(); j++){
            term_counts[terms[j]]++;
        }

        for (auto it =term_counts.begin(); it !=term_counts.end(); it++){ //add postings for each term in chunk
            const std::string& term = it->first;
            std::size_t count =it->second;

            postings_[term].push_back(Posting{i, count});
        }
    }
}

std::size_t CorpusIndex::document_frequency(
    const std::string& normalized_term) const {
    // TODO: return how many chunks contain the requested term.
    std::vector<std::string> t = TextProcessor::terms(normalized_term);
    if(t.size()>1){
        throw std::invalid_argument("multi token terms invalid");
    }
    if(t.empty()){
        return 0;
    }
    
    auto it =postings_.find(t[0]);
    if (it==postings_.end()){
        return 0;
    }
    return it->second.size();
}

std::size_t CorpusIndex::term_frequency(    
    const std::string& normalized_term,
    const std::string& chunk_id) const {
    // TODO: return the requested term's frequency in the specified chunk.
    std::vector<std::string> t = TextProcessor::terms(normalized_term);
    if(t.size()>1){
        throw std::invalid_argument("multi token terms invalid");
    }
    if(t.empty()){
        return 0;
    }
    
    auto chunk_it =chunk_by_id_.find(chunk_id);
    if(chunk_it ==chunk_by_id_.end()){
        return 0;
    }

    std::size_t target_chunk_index = chunk_it->second;

    auto posting_it = postings_.find(t[0]);
    if(posting_it ==postings_.end()){
        return 0;
    }

    const auto& postings_list =posting_it->second;
    for(std::size_t i=0; i<postings_list.size(); i++){
        if(postings_list[i].chunk_index ==target_chunk_index){
            return postings_list[i].frequency;
        }
    }
    return 0;
}

const std::vector<CorpusIndex::Posting>* CorpusIndex::postings(
    const std::string& normalized_term) const noexcept {
    // TODO: return the postings associated with the requested term.
    auto it=postings_.find(normalized_term);
    if (it==postings_.end()){
        return nullptr;
    }
    return &(it->second);
}

const Chunk* CorpusIndex::find_chunk(
    const std::vector<Chunk>& chunks,
    const std::string& chunk_id) const noexcept {
    // TODO: find the chunk identified by the requested chunk ID.
    auto it= chunk_by_id_.find(chunk_id);
    if (it==chunk_by_id_.end()){
        return nullptr;
    }
    std::size_t idx =it->second;
    if(idx>=chunks.size()){
        return nullptr;
    }
    return &chunks[idx];
}

std::size_t CorpusIndex::chunk_index(const std::string& chunk_id) const {
    // TODO: return the stored index of the requested chunk ID.
    auto it=chunk_by_id_.find(chunk_id);
    if (it==chunk_by_id_.end()){
        throw std::out_of_range("chunk id not found at index: " +chunk_id);
    }
    return it->second;
}

}  // namespace aiws
