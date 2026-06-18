from dataclasses import dataclass, field

# @dataclass
# class RepeatPerfCounters:
#     # raw data
#     cycles: int
#     instructions: int
#     branches: int
#     branch_misses: int
#     cache_references: int
#     cache_misses: int
#
#     # Must be N from config.json, not applied_msgs — applied_msgs is 0 in null/decode (div-by-zero)
#     msgs: int

@dataclass
class CounterMetrics:
    ipc: float
    cycle_per_msg: float
    cache_miss_rate: float
    branch_miss_rate: float

    # Source: exchange's send_throughput_msgs_per_s RESULT line
    throughput: int

@dataclass
class RepeatIntervals:
    # ex
    ex_intvl_send: list[int] = field(default_factory=list)
    # gw
    gw_intvl_decode: list[int] = field(default_factory=list)
    gw_intvl_send: list[int] = field(default_factory=list)
    # lob
    lob_intvl_decode: list[int] = field(default_factory=list)
    lob_intvl_apply: list[int] = field(default_factory=list)
    # transport
    ex2gw_trans: list[int] = field(default_factory=list)
    gw2lob_trans: list[int] = field(default_factory=list)

    # Only exist in match mode
    w2w: list[int] = field(default_factory=list)

@dataclass
class RepeatIntervalMetrics:
    # ex
    ex_intvl_send_p50: int; ex_intvl_send_p99: int; ex_intvl_send_max: int; ex_intvl_send_mean: float
    # gw
    gw_intvl_decode_p50: int; gw_intvl_decode_p99: int; gw_intvl_decode_max: int; gw_intvl_decode_mean: float

    gw_intvl_send_p50: int; gw_intvl_send_p99: int; gw_intvl_send_max: int; gw_intvl_send_mean: float
    # lob
    lob_intvl_decode_p50: int; lob_intvl_decode_p99: int; lob_intvl_decode_max: int; lob_intvl_decode_mean: float

    lob_intvl_apply_p50: int; lob_intvl_apply_p99: int; lob_intvl_apply_max: int; lob_intvl_apply_mean: float
    # transport
    ex2gw_trans_p50: int; ex2gw_trans_p99: int; ex2gw_trans_max: int; ex2gw_trans_mean: float

    gw2lob_trans_p50: int; gw2lob_trans_p99: int; gw2lob_trans_max: int; gw2lob_trans_mean: float

    w2w_p50: int; w2w_p99: int; w2w_max: int; w2w_mean: float

@dataclass
class LatencyCell:
    """
    Perf optimization will around the matching hot path, so we care about the improvement of lob_apply and w2w.
    For other interval stats, like send and decode, they are negative controls which should not move when we opt the matching hot path.
    Otherwise, "engine get improved" is actually a system-wide perturbation (thermal, governor, noise) — the controls are how to prove the change is localized.
    """

    # [mean, MAD, cv] # todo! change list[val] to dict[stat, val]
    # ex
    ex_intvl_send_p50_stat: list[float] = field(default_factory=list)
    ex_intvl_send_p99_stat: list[float] = field(default_factory=list)
    ex_intvl_send_max_stat: list[float] = field(default_factory=list)
    ex_intvl_send_mean_stat: list[float] = field(default_factory=list)
    # gw
    gw_intvl_decode_p50_stat: list[float] = field(default_factory=list)
    gw_intvl_decode_p99_stat: list[float] = field(default_factory=list)
    gw_intvl_decode_max_stat: list[float] = field(default_factory=list)
    gw_intvl_decode_mean_stat: list[float] = field(default_factory=list)

    gw_intvl_send_p50_stat: list[float] = field(default_factory=list)
    gw_intvl_send_p99_stat: list[float] = field(default_factory=list)
    gw_intvl_send_max_stat: list[float] = field(default_factory=list)
    gw_intvl_send_mean_stat: list[float] = field(default_factory=list)
    # lob
    lob_intvl_decode_p50_stat: list[float] = field(default_factory=list)
    lob_intvl_decode_p99_stat: list[float] = field(default_factory=list)
    lob_intvl_decode_max_stat: list[float] = field(default_factory=list)
    lob_intvl_decode_mean_stat: list[float] = field(default_factory=list)

    lob_intvl_apply_p50_stat: list[float] = field(default_factory=list)
    lob_intvl_apply_p99_stat: list[float] = field(default_factory=list)
    lob_intvl_apply_max_stat: list[float] = field(default_factory=list)
    lob_intvl_apply_mean_stat: list[float] = field(default_factory=list)
    # transport
    ex2gw_trans_p50_stat: list[float] = field(default_factory=list)
    ex2gw_trans_p99_stat: list[float] = field(default_factory=list)
    ex2gw_trans_max_stat: list[float] = field(default_factory=list)
    ex2gw_trans_mean_stat: list[float] = field(default_factory=list)

    gw2lob_trans_p50_stat: list[float] = field(default_factory=list)
    gw2lob_trans_p99_stat: list[float] = field(default_factory=list)
    gw2lob_trans_max_stat: list[float] = field(default_factory=list)
    gw2lob_trans_mean_stat: list[float] = field(default_factory=list)

    w2w_p50_stat: list[float] = field(default_factory=list)
    w2w_p99_stat: list[float] = field(default_factory=list)
    w2w_max_stat: list[float] = field(default_factory=list)
    w2w_mean_stat: list[float] = field(default_factory=list)

@dataclass
class PerfCell:
    # perf counters
    ipc_stat: list[float] = field(default_factory=list)
    cycle_per_msg_stat: list[float] = field(default_factory=list)
    cache_miss_rate_stat: list[float] = field(default_factory=list)
    branch_miss_rate_stat: list[float] = field(default_factory=list)

    throughput_stat: list[float] = field(default_factory=list)