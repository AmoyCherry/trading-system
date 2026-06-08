from dataclasses import dataclass, field

@dataclass
class RepeatPerfStat:
    # raw data
    cycles: int
    instructions: int
    branches: int
    branch_misses: int
    cache_references: int
    cache_misses: int

    # Must be N from config.json, not applied_msgs — applied_msgs is 0 in null/decode (div-by-zero)
    msgs: int

    ipc: float
    cycle_per_msg: float
    cache_miss_rate: float
    branch_miss_rate: float

@dataclass
class RepeatIntervals:
    # ex
    ex_intvl_send: list[int]
    # gw
    gw_intvl_decode: list[int]
    gw_intvl_send: list[int]
    # lob
    lob_intvl_decode: list[int]
    lob_intvl_apply: list[int]
    # transport
    ex2gw_trans: list[int]
    gw2lob_trans: list[int]
    w2w_trans: list[int]

@dataclass
class RepeatIntervalStat:
    # ex
    ex_intvl_send_p50: int
    ex_intvl_send_p99: int
    ex_intvl_send_max: int
    # gw
    gw_intvl_decode_p50: int
    gw_intvl_decode_p99: int
    gw_intvl_decode_max: int

    gw_intvl_send_p50: int
    gw_intvl_send_p99: int
    gw_intvl_send_max: int
    # lob
    lob_intvl_decode_p50: int
    lob_intvl_decode_p99: int
    lob_intvl_decode_max: int

    lob_intvl_apply_p50: int
    lob_intvl_apply_p99: int
    lob_intvl_apply_max: int
    # transport
    ex2gw_trans_p50: int
    ex2gw_trans_p99: int
    ex2gw_trans_max: int

    gw2lob_trans_p50: int
    gw2lob_trans_p99: int
    gw2lob_trans_max: int

    w2w_p50: int
    w2w_p99: int
    w2w_max: int

    # Source: exchange's send_throughput_msgs_per_s RESULT line
    throughput: int

@dataclass
class Cell:
    """
    Perf optimization will around the matching hot path, so we care about the improvement of lob_apply and w2w.
    For other interval stats, like send and decode, they are negative controls which should not move when we opt the matching hot path.
    Otherwise, "engine get improved" is actually a system-wide perturbation (thermal, governor, noise) — the controls are how to prove the change is localized.
    """

    intervalStat: list[RepeatIntervalStat] = field(default_factory=list)
    perfStat: list[RepeatPerfStat] = field(default_factory=list)

    # [mean, MAD, cv]
    # ex
    ex_intvl_send_stats_p50: list[fload] = field(default_factory=list)
    ex_intvl_send_stats_p99: list[fload] = field(default_factory=list)
    ex_intvl_send_stats_max: list[fload] = field(default_factory=list)
    # gw
    gw_intvl_decode_stats_p50: list[fload] = field(default_factory=list)
    gw_intvl_decode_stats_p99: list[fload] = field(default_factory=list)
    gw_intvl_decode_stats_max: list[fload] = field(default_factory=list)

    gw_intvl_send_stats_p50: list[fload] = field(default_factory=list)
    gw_intvl_send_stats_p99: list[fload] = field(default_factory=list)
    gw_intvl_send_stats_max: list[fload] = field(default_factory=list)
    # lob
    lob_intvl_decode_stats_p50: list[fload] = field(default_factory=list)
    lob_intvl_decode_stats_p99: list[fload] = field(default_factory=list)
    lob_intvl_decode_stats_max: list[fload] = field(default_factory=list)

    lob_intvl_apply_stats_p50: list[fload] = field(default_factory=list)
    lob_intvl_apply_stats_p99: list[fload] = field(default_factory=list)
    lob_intvl_apply_stats_max: list[fload] = field(default_factory=list)
    # transport
    ex2gw_trans_stats_p50: list[fload] = field(default_factory=list)
    ex2gw_trans_stats_p99: list[fload] = field(default_factory=list)
    ex2gw_trans_stats_max: list[fload] = field(default_factory=list)

    gw2lob_trans_stats_p50: list[fload] = field(default_factory=list)
    gw2lob_trans_stats_p99: list[fload] = field(default_factory=list)
    gw2lob_trans_stats_max: list[fload] = field(default_factory=list)

    w2w_trans_stats_p50: list[fload] = field(default_factory=list)
    w2w_trans_stats_p99: list[fload] = field(default_factory=list)
    w2w_trans_stats_max: list[fload] = field(default_factory=list)

    # perf counters
    ipc: list[float] = field(default_factory=list)
    cycle_per_msg: list[float] = field(default_factory=list)
    cache_miss_rate: list[float] = field(default_factory=list)
    branch_miss_rate: list[float] = field(default_factory=list)
