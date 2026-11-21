#pragma once
#include <array>
#include <cstdint>
#include <deque>

class ResolverLearning {
public:
    enum class Outcome : std::uint8_t { HIT, MISS, INCONCLUSIVE };
    struct Observation {
        float m_sim_time;
        float m_body_yaw;
        float m_speed_2d;
        int m_flags;
        std::uint32_t m_animation_signature;
    };
    struct Statistics {
        std::size_t m_observations;
        std::uint32_t m_hits, m_misses, m_inconclusive;
        float m_evidence_confidence;
    };
private:
    struct PlayerHistory {
        std::deque<Observation> m_observations;
        std::uint32_t m_hits{ 0 }, m_misses{ 0 }, m_inconclusive{ 0 };
    };
    std::array<PlayerHistory, 64> m_players{};
public:
    void Observe(int player_index, const Observation& observation);
    void RecordOutcome(int player_index, Outcome outcome);
    void Reset(int player_index);
    void ResetAll();
    Statistics GetStatistics(int player_index) const;
    const std::deque<Observation>* GetObservations(int player_index) const;
};
extern ResolverLearning g_resolver_learning;
