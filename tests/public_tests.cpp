#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "aiws/text_processor.hpp"
#include "aiws/chunker.hpp"
#include "aiws/retrieval_engine.hpp"
#include "aiws/context_builder.hpp"
#include "aiws/processing_core.hpp"
#include "aiws/workspace.hpp"
#include "aiws/document.hpp"

#include <stdexcept>
#include <string>
#include <vector>

TEST_CASE("text processor normalization and tokenization"){
    std::string empty_norm =aiws::TextProcessor::normalize("");
    REQUIRE(empty_norm.empty());

    std::vector<std::string> empty_terms = aiws::TextProcessor::terms("");
    REQUIRE(empty_terms.empty());

    std::string text =" machine Learning uses, Data!    ";
    std::string normalized = aiws::TextProcessor::normalize(text);
    REQUIRE( normalized== "machine learning uses data");

    std::vector<std::string> terms = {"alice", "bob", "charlie"};
    std::string joined = aiws::TextProcessor::join(terms, 0, 10);
    REQUIRE(joined =="alice bob charlie");
}

TEST_CASE("chunker policy and processing core initial state"){
    aiws::ChunkingPolicy policy{120,20,20};
    aiws::Chunker chunker(policy);

    REQUIRE(policy.max_tokens ==120);
    REQUIRE(policy.overlap==20);
    REQUIRE(policy.paragraph_window==20);

    aiws::ProcessingCore core;
    REQUIRE(core.chunk_count()==0);
    REQUIRE(core.chunks().empty());
}

TEST_CASE("corpus index document frequency"){
    aiws::Chunk c1{};
    c1.id="doc1#0";
    c1.document_id = "doc1";
    c1.document_order = 0;
    c1.sequence=0;
    c1.text = "machine learning data quality";
    c1.token_count = 4;

    aiws::Chunk c2{};
    c2.id = "doc1#1";
    c2.document_id = "doc1";
    c2.document_order=0;
    c2.sequence=1;
    c2.text="data analytics results search";

    std::vector<aiws::Chunk> chunks = {c1, c2};
    aiws::CorpusIndex index(chunks);

    REQUIRE(index.document_frequency("data") ==2);
    REQUIRE(index.document_frequency("learning")==1);
    REQUIRE(index.document_frequency("nonexistent")==0);
}

TEST_CASE("retrieval engine search"){
    aiws::Chunk c1{};
    c1.id="doc1#0";
    c1.document_id = "doc1";
    c1.document_order = 0;
    c1.sequence=0;
    c1.text = "machine learning data quality";
    c1.token_count = 4;

    aiws::Chunk c2{};
    c2.id = "doc1#1";
    c2.document_id = "doc1";
    c2.document_order=0;
    c2.sequence=1;
    c2.text="data analytics results search";

    std::vector<aiws::Chunk> chunks = {c1, c2};
    aiws::CorpusIndex index(chunks);
    aiws::RetrievalEngine engine;

    std::vector<aiws::SearchResult> results =engine.search("data quality", 5, chunks, index);
    REQUIRE(results.size()>0);
    REQUIRE(results[0].chunk_id=="doc1#0");

    REQUIRE_THROWS_AS(engine.search("data", -1, chunks, index), std::invalid_argument);
}

TEST_CASE("context builder token budget"){
    aiws::SearchResult result {};
    result.chunk_id = "doc1#0";
    result.document_id = "doc1";
    result.chunk_sequence = 0;
    result.text = "one two three four five";
    result.score =1.0;

    std::vector<aiws::SearchResult> results = {result};

    aiws::ContextBuilder builder;
    auto context = builder.build(results, 3);
    REQUIRE(context.size()==1);
    REQUIRE(context[0].token_count==3);
    REQUIRE(context[0].truncated==true);
}

TEST_CASE("multi doc processing core"){
    aiws::Document doc_a("doc_a", "Title A", "programming in c++");
    aiws::Document doc_b("doc_b", "Title B", "data structures and algorithms");

    aiws::Workspace ws;
    ws.add_document(doc_a);
    ws.add_document(doc_b);

    aiws::ProcessingCore core;
    core.rebuild(ws);

    REQUIRE(core.chunk_count()>=2);
    auto results= core.search("programming c++", 5);
    REQUIRE(results.size()>0);
    REQUIRE(results[0].document_id =="doc_a");
}