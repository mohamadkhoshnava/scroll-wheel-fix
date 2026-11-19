import Foundation

var checks = 0
func check(_ condition: Bool, _ label: String) {
    guard condition else {
        fputs("FAIL: \(label)\n", stderr)
        exit(1)
    }
    checks += 1
    print("PASS: \(label)")
}

// Normal movement and noisy one/two-event direction reversals.
var gate = DirectionGate()
check(gate.process(sign: 1, time: 0) == .pass, "first wheel tick passes")
check(gate.process(sign: 1, time: 0.05) == .pass, "same direction passes immediately")
check(gate.process(sign: -1, time: 0.06) == .hold, "isolated reversal is held")
check(gate.process(sign: 1, time: 0.08) == .pass && gate.candidateCount == 0,
      "return to original direction discards candidate")
check(gate.process(sign: -1, time: 0.09) == .hold, "short reverse burst begins")
check(gate.process(sign: -1, time: 0.096) == .hold, "second tick alone cannot confirm a fast burst")
check(gate.process(sign: 1, time: 0.11) == .pass, "two-event bounce is rejected")

// Intended reversal must last for the confirmation window and have >= 2 ticks.
check(gate.process(sign: -1, time: 0.15) == .hold, "intentional reversal begins held")
check(gate.process(sign: -1, time: 0.25) == .confirm, "sustained reversal is confirmed")
check(gate.process(sign: -1, time: 0.28) == .pass, "new direction then passes immediately")
check(gate.process(sign: 1, time: 1.0) == .pass, "idle gap permits a new direction immediately")
gate.reset()
check(gate.direction == 0 && gate.candidateCount == 0 && gate.candidateStart == nil,
      "reset clears a pending direction")
check(gate.process(sign: 0, time: 1.1) == .pass && gate.direction == 0,
      "zero delta does not choose a direction")

// Strong mode requires a longer sustained reversal than balanced mode.
var balanced = DirectionGate()
var strong = DirectionGate()
strong.window = 0.140
for time in [2.0, 2.02] {
    _ = balanced.process(sign: -1, time: time)
    _ = strong.process(sign: -1, time: time)
}
_ = balanced.process(sign: 1, time: 2.03)
_ = strong.process(sign: 1, time: 2.03)
check(balanced.process(sign: 1, time: 2.13) == .confirm, "balanced confirms after 100 ms")
check(strong.process(sign: 1, time: 2.13) == .hold, "strong still holds after 100 ms")
check(strong.process(sign: 1, time: 2.20) == .confirm, "strong confirms after 170 ms")

// Synthetic end-to-end direction sequence: preserve accepted events, discard
// pending bursts on return, and replay pending ticks on a sustained reversal.
struct Sample {
    var sign: Int
    var time: Double
}
let samples: [Sample] = [
    .init(sign: 1, time: 3.0), .init(sign: 1, time: 3.05),
    .init(sign: -1, time: 3.06), .init(sign: -1, time: 3.065),
    .init(sign: 1, time: 3.08), .init(sign: 1, time: 3.12),
    .init(sign: -1, time: 3.14), .init(sign: -1, time: 3.25),
    .init(sign: -1, time: 3.28)
]
var replayGate = DirectionGate()
var buffered: [Int] = []
var output: [Int] = []
for sample in samples {
    switch replayGate.process(sign: sample.sign, time: sample.time) {
    case .pass:
        buffered.removeAll()
        output.append(sample.sign)
    case .hold:
        buffered.append(sample.sign)
    case .confirm:
        output.append(contentsOf: buffered)
        buffered.removeAll()
        output.append(sample.sign)
    }
}
check(output == [1, 1, 1, 1, -1, -1, -1],
      "synthetic replay removes bounce and keeps confirmed reversal ticks")
print("All \(checks) checks passed. No Accessibility permission was used.")
