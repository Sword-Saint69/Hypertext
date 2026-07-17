// test_prediction.cpp — Unit tests for Phase 6 Prediction Engine

#include <gtest/gtest.h>
#include <hypercore/core/CharacterPredictor.hpp>
#include <hypercore/core/ContextMixer.hpp>

using namespace hypercore;

TEST(CharacterPredictor, BasicPrediction) {
    CharacterPredictor predictor;
    PredictorContext ctx;
    ctx.island = IslandType::Binary;
    ctx.position = 0;

    // Initially, it should be uniform (1.0 / 256.0)
    ByteDistribution dist = predictor.predict(ctx);
    EXPECT_NEAR(dist[0], 1.0f / 256.0f, 0.0001f);
    EXPECT_NEAR(dist[255], 1.0f / 256.0f, 0.0001f);

    // Let's feed it "ab", "ac", "ab"
    // So if history is "a", next byte is 'b' (twice) and 'c' (once)
    u8 history[] = {'a'};
    ctx.history = ByteSpan(history, 1);

    predictor.update(ctx, 'b');
    predictor.update(ctx, 'c');
    predictor.update(ctx, 'b');

    dist = predictor.predict(ctx);
    
    // total count = 256 (initial 1s) + 3 (updates) = 259
    // count for 'b' = 1 + 2 = 3
    // count for 'c' = 1 + 1 = 2
    // count for 'd' = 1
    
    EXPECT_NEAR(dist['b'], 3.0f / 259.0f, 0.0001f);
    EXPECT_NEAR(dist['c'], 2.0f / 259.0f, 0.0001f);
    EXPECT_NEAR(dist['d'], 1.0f / 259.0f, 0.0001f);
}

TEST(ContextMixer, UniformFallbackWhenEmpty) {
    ContextMixer mixer;
    PredictorContext ctx;
    
    ByteDistribution dist = mixer.predict(ctx);
    EXPECT_NEAR(dist[42], 1.0f / 256.0f, 0.0001f);
}

// A dummy predictor that just votes 100% for a specific byte
class DummyPredictor : public IPredictor {
public:
    explicit DummyPredictor(u8 target) : m_target(target) {}
    
    ByteDistribution predict(const PredictorContext& /*ctx*/) override {
        ByteDistribution d;
        d[m_target] = 1.0f;
        return d;
    }
    
    void update(const PredictorContext& /*ctx*/, u8 /*actual*/) override {}
    void reset() noexcept override {}
    PredictorType type() const noexcept override { return PredictorType::Character; }
    std::string_view name() const noexcept override { return "Dummy"; }
    u32 order() const noexcept override { return 0; }
    u64 memory_bytes() const noexcept override { return 0; }
    
private:
    u8 m_target;
};

TEST(ContextMixer, AveragingPredictors) {
    ContextMixer mixer;
    mixer.add_predictor(std::make_unique<DummyPredictor>('X'));
    mixer.add_predictor(std::make_unique<DummyPredictor>('Y'));
    
    PredictorContext ctx;
    ByteDistribution dist = mixer.predict(ctx);
    
    // It should perfectly average them: 50% X, 50% Y, 0% everything else
    EXPECT_NEAR(dist['X'], 0.5f, 0.0001f);
    EXPECT_NEAR(dist['Y'], 0.5f, 0.0001f);
    EXPECT_NEAR(dist['Z'], 0.0f, 0.0001f);
}
