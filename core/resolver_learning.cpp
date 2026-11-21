#include "includes.h"
ResolverLearning g_resolver_learning{ };

void ResolverLearning::Observe(int player_index, const Observation& observation) {
    if (player_index < 1 || player_index > static_cast<int>(m_players.size()))
        return;
    if (!std::isfinite(observation.m_sim_time) || !std::isfinite(observation.m_body_yaw) ||
        !std::isfinite(observation.m_speed_2d))
        return;
    PlayerHistory& history = m_players[player_index - 1];
    // Do not require monotonic simtime: the 2018 protocol can legitimately reset it.
    if (!history.m_observations.empty()) {
        const Observation& previous = history.m_observations.front();
        if (previous.m_sim_time == observation.m_sim_time &&
            previous.m_animation_signature == observation.m_animation_signature)
            return;
    }
    history.m_observations.push_front(observation);
    while (history.m_observations.size() > 64)
        history.m_observations.pop_back();
}

void ResolverLearning::RecordOutcome(int player_index, Outcome outcome) {
    if (player_index < 1 || player_index > static_cast<int>(m_players.size()))
        return;
    PlayerHistory& history = m_players[player_index - 1];
    switch (outcome) {
    case Outcome::HIT: ++history.m_hits; break;
    case Outcome::MISS: ++history.m_misses; break;
    case Outcome::INCONCLUSIVE: ++history.m_inconclusive; break;
    }
}

void ResolverLearning::Reset(int player_index) {
    if (player_index >= 1 && player_index <= static_cast<int>(m_players.size()))
        m_players[player_index - 1] = PlayerHistory{};
}

void ResolverLearning::ResetAll() {
    for (auto& player : m_players)
        player = PlayerHistory{};
}

ResolverLearning::Statistics ResolverLearning::GetStatistics(int player_index) const {
    Statistics result{};
    if (player_index < 1 || player_index > static_cast<int>(m_players.size()))
        return result;
    const PlayerHistory& history = m_players[player_index - 1];
    result.m_observations = history.m_observations.size();
    result.m_hits = history.m_hits;
    result.m_misses = history.m_misses;
    result.m_inconclusive = history.m_inconclusive;
    // Evidence volume only; this is not confidence in a guessed angle.
    result.m_evidence_confidence = std::min(1.f, static_cast<float>(result.m_observations) / 12.f);
    return result;
}

const std::deque<ResolverLearning::Observation>*
ResolverLearning::GetObservations(int player_index) const {
    if (player_index < 1 || player_index > static_cast<int>(m_players.size()))
        return nullptr;
    return &m_players[player_index - 1].m_observations;
}
