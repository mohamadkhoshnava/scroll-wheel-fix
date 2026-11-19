// Confirms sustained direction changes and suppresses brief reversal bursts.
struct DirectionGate {
    var direction = 0
    var lastAccepted = -Double.infinity
    var candidateStart: Double?
    var candidateCount = 0
    var window = 0.080
    enum Decision { case pass, hold, confirm }
    mutating func reset() {
        direction = 0; lastAccepted = -Double.infinity
        candidateStart = nil; candidateCount = 0
    }
    mutating func process(sign: Int, time: Double) -> Decision {
        guard sign != 0 else { return .pass }
        if direction == 0 || time - lastAccepted > 0.60 {
            direction = sign; lastAccepted = time
            candidateStart = nil; candidateCount = 0
            return .pass
        }
        if sign == direction {
            lastAccepted = time; candidateStart = nil; candidateCount = 0
            return .pass
        }
        if candidateStart == nil { candidateStart = time }
        candidateCount += 1
        if candidateCount >= 2 && time - candidateStart! >= window {
            direction = sign; lastAccepted = time
            candidateStart = nil; candidateCount = 0
            return .confirm
        }
        return .hold
    }
}
