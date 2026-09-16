#include "aiws/processing_core.hpp"
#include "aiws/text_processor.hpp"
#include "aiws/chunker.hpp"
#include "aiws/corpus_index.hpp"
#include "aiws/retrieval_engine.hpp"
#include "aiws/context_builder.hpp"

#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace aiws {

struct ProcessingCore::Impl {
    // TODO: define the internal state used by the processing core.

    std::vector<Chunk> chunks;
    CorpusIndex index;
    RetrievalEngine retrieval_engine;
    ContextBuilder context_builder;
};

ProcessingCore::ProcessingCore() : impl_(std::make_unique<Impl>()) { }

ProcessingCore::~ProcessingCore() = default;

ProcessingCore::ProcessingCore(ProcessingCore&&) noexcept = default;

ProcessingCore& ProcessingCore::operator=(ProcessingCore&&) noexcept = default;

std::string ProcessingCore::normalize(const std::string& text) {
    // TODO: return the normalized form of the input text.
    return TextProcessor::normalize(text);
}

void ProcessingCore::rebuild(const Workspace& workspace) {
    // TODO: rebuild the processing state from the workspace.
    const auto& docs = workspace.documents();

    std::unordered_set<std::string> seen_doc_ids;
    for (std::size_t i=0; i<docs.size(); i++){
        if (seen_doc_ids.find(docs[i].id()) != seen_doc_ids.end()){
            throw std::invalid_argument("Duplicate document id in workspace: " + docs[i].id());
        }
        seen_doc_ids.insert(docs[i].id());
    }

    ChunkingPolicy policy{
        kMaxChunkTokens,
        kChunkOverlap,
        kParagraphPreferenceWindow
    };
    Chunker chunker(policy);
    std::vector<Chunk> new_chunks;

    for (std::size_t i=0; i<docs.size(); i++){
        std::vector<Chunk> doc_chunks=chunker.chunk(docs[i], i);
        for (std::size_t j=0; j<doc_chunks.size(); j++){
            new_chunks.push_back(std::move(doc_chunks[j]));
        }
    }

    CorpusIndex new_index(new_chunks);

    impl_->chunks = std::move(new_chunks);
    impl_->index=std::move(new_index);
}

const std::vector<Chunk>& ProcessingCore::chunks() const noexcept {
    static const std::vector<Chunk> empty;

    // TODO: return the chunks currently stored by the processing core.
    return impl_->chunks;
}

std::size_t ProcessingCore::chunk_count() const noexcept {
    // TODO: return the number of stored chunks.
    return impl_->chunks.size();
}

std::size_t ProcessingCore::document_frequency(const std::string& term) const {
    // TODO: return the document frequency for the requested term.
    std::string normalized_term = TextProcessor::normalize(term);
    return impl_->index.document_frequency(normalized_term);
}

std::size_t ProcessingCore::term_frequency(const std::string& term,
                                           const std::string& chunk_id) const {
    // TODO: return the term frequency for the requested chunk.
    std::string normalized_term=TextProcessor::normalize(term);
    return impl_->index.term_frequency(normalized_term, chunk_id);
}

std::vector<SearchResult> ProcessingCore::search(const std::string& query, int k) const {
    // TODO: return the ranked results for the requested query.
    return impl_->retrieval_engine.search(query, k, impl_->chunks, impl_->index);
}

std::vector<ContextItem> ProcessingCore::build_context(const std::string& query,
                                                       int k,
                                                       std::size_t token_budget) const {
    // TODO: build bounded context for the requested query.
    std::vector<SearchResult> search_results = search(query,k);
    return impl_->context_builder.build(search_results, token_budget);
}

}  // namespace aiws
