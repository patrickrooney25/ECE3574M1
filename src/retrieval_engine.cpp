#include "aiws/retrieval_engine.hpp"
#include "aiws/text_processor.hpp"

#include <cmath>
#include <stdexcept>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>

namespace aiws {

double RetrievalEngine::canonical_score(double value) {
    
    return std::round(value*1e12)/ 1e12;
}

std::vector<SearchResult> RetrievalEngine::search(const std::string& query,
                                                  int k,
                                                  const std::vector<Chunk>& chunks,
                                                  const CorpusIndex& index) const {
    if (k<0){ //negative is invalid
        throw std::invalid_argument("k can't be negatice");
    }
    if (k==0 ||chunks.empty()){ //no results
        return {};
    }

    std::vector<std::string> raw_terms =TextProcessor::terms(query);
    if (raw_terms.empty()){  //normalize and extract unique query terms
        return{};
    }

    std::vector<std::string> unique_qterms;
    std::unordered_set<std::string> seen_qterms;
    for (std::size_t i=0; i<raw_terms.size(); i++){
        if (seen_qterms.find(raw_terms[i]) ==seen_qterms.end()){
            seen_qterms.insert(raw_terms[i]);
            unique_qterms.push_back(raw_terms[i]);
        }
    }
    if(unique_qterms.empty()){
        return {};
    }

    //track for each candidate chunk, base score sum and count of matched query temrs
    const double N = static_cast<double>(chunks.size());
    const double Q = static_cast<double>(unique_qterms.size());
    std::unordered_map<std::size_t, double> base_scores;
    std::unordered_map<std::size_t, std::size_t> matched_counts;

    for (std::size_t i=0; i<unique_qterms.size(); i++){
        const std::string& term = unique_qterms[i];
        const std::vector<CorpusIndex::Posting>* postings_ptr =index.postings(term);

        if (postings_ptr !=nullptr && !postings_ptr->empty()){
            double df = static_cast<double>(index.document_frequency(term));
            double idf = std::log((N+1.0)/(df+1.0) +1.0);

            const std::vector<CorpusIndex::Posting>& postings_list = *postings_ptr;
            for (std::size_t j=0; j<postings_list.size(); j++){
                std::size_t chunk_idx = postings_list[j].chunk_index;
                double tf =1.0 +std::log(static_cast<double>(postings_list[j].frequency));

                base_scores[chunk_idx]+=tf*idf;
                matched_counts[chunk_idx]+=1;
            }
        }
    }

    if (base_scores.empty()){
        return {};
    }

    struct Candidate {
        std::size_t chunk_idx;
        double score;
    };

    //build array for candidate chunks
    std::vector<Candidate> candidates;
    candidates.reserve(base_scores.size());

    for (auto it=base_scores.begin(); it!=base_scores.end(); it++){
        std::size_t chunk_idx=it->first;
        double base_sc=it->second;
        double matched =static_cast<double>(matched_counts[chunk_idx]);

        double coverage=1.0 + .10 *(matched/Q);
        double raw_score =base_sc*coverage;
        double final_score=canonical_score(raw_score);

        candidates.push_back(Candidate{chunk_idx, final_score});

    }


    //sort descending score, ascending source document insertion order, ascending chunk sequence number
    std::sort(candidates.begin(), candidates.end(), [&chunks](const Candidate& a, const Candidate& b){
        if (std::abs(a.score -b.score)> 1e-12){
            return a.score > b.score;
        }
        const Chunk& chunk_a=chunks[a.chunk_idx];
        const Chunk& chunk_b = chunks[b.chunk_idx];

        if(chunk_a.document_order != chunk_b.document_order){
            return chunk_a.document_order < chunk_b.document_order;
        }
        return chunk_a.sequence <chunk_b.sequence;
    });

    //return no more than k results
    if (candidates.size() > static_cast<std::size_t>(k)){
        candidates.resize(static_cast<std::size_t>(k));
    }

    std::vector<SearchResult> results;
    results.reserve(candidates.size());
    for (std::size_t i=0; i<candidates.size(); i++){
        std::size_t c_idx = candidates[i].chunk_idx;
        const Chunk& c = chunks[c_idx];
        
        SearchResult res{};
        res.chunk_id=c.id;
        res.document_id=c.document_id;
        res.chunk_sequence=c.sequence;
        res.text=c.text;
        res.score = candidates[i].score;
        res.matched_terms = matched_counts[c_idx];

        results.push_back(res);
    }

    return results;
}

}  // namespace aiws

