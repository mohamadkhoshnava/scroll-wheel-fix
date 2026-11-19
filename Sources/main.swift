import AppKit
import ApplicationServices
import CoreGraphics

// Only wheel events are requested. No keyboard, click, or pointer stream is read.
final class ScrollFix: NSObject, NSApplicationDelegate {
    var item: NSStatusItem!
    var tap: CFMachPort?
    var source: CFRunLoopSource?
    var timer: Timer?
    var window: NSWindow?
    var info: NSTextField?
    var gate = DirectionGate()
    var pending: [CGEvent] = []
    var enabled = true
    var wheelSeen = 0
    var continuousSeen = 0
    var dropped = 0
    var statusText = "Waiting for Accessibility permission"

    func applicationDidFinishLaunching(_ notification: Notification) {
        NSApp.setActivationPolicy(.accessory)
        item = NSStatusBar.system.statusItem(withLength: NSStatusItem.variableLength)
        item.button?.title = "SWF ○"
        item.button?.toolTip = "Scroll Wheel Fix — filter wheel direction glitches"
        refreshMenu()
        connectIfAllowed()
        if tap == nil { showHelp(nil) }
        timer = Timer.scheduledTimer(withTimeInterval: 2, repeats: true) { [weak self] _ in
            self?.connectIfAllowed(); self?.refreshMenu()
        }
    }
    func applicationWillTerminate(_ notification: Notification) { disconnect() }
    func applicationShouldHandleReopen(_ sender: NSApplication, hasVisibleWindows flag: Bool) -> Bool {
        showHelp(nil)
        return true
    }
    func discardPending() { dropped += pending.count; pending.removeAll() }
    func disconnect() {
        if let tap { CGEvent.tapEnable(tap: tap, enable: false); CFMachPortInvalidate(tap) }
        if let source { CFRunLoopRemoveSource(CFRunLoopGetMain(), source, .commonModes) }
        tap = nil; source = nil; discardPending(); gate.reset()
    }
    func connectIfAllowed() {
        guard enabled else { return }
        guard AXIsProcessTrusted() else {
            if tap != nil { disconnect() }
            statusText = "Waiting for Accessibility permission"; return
        }
        if let tap {
            if !CGEvent.tapIsEnabled(tap: tap) {
                discardPending(); gate.reset(); CGEvent.tapEnable(tap: tap, enable: true)
            }
            statusText = "Filter active"; return
        }
        let mask = CGEventMask(1) << CGEventType.scrollWheel.rawValue
        tap = CGEvent.tapCreate(tap: .cgSessionEventTap, place: .headInsertEventTap,
            options: .defaultTap, eventsOfInterest: mask,
            callback: { proxy, type, event, context in
                guard let context else { return Unmanaged.passUnretained(event) }
                let app = Unmanaged<ScrollFix>.fromOpaque(context).takeUnretainedValue()
                return app.handle(proxy: proxy, type: type, event: event)
            }, userInfo: Unmanaged.passUnretained(self).toOpaque())
        guard let tap else { statusText = "Permission granted; event filter unavailable. Restart app."; return }
        source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, tap, 0)
        CFRunLoopAddSource(CFRunLoopGetMain(), source, .commonModes)
        CGEvent.tapEnable(tap: tap, enable: true)
        statusText = "Filter active"
    }
    func handle(proxy: CGEventTapProxy, type: CGEventType, event: CGEvent) -> Unmanaged<CGEvent>? {
        if type == .tapDisabledByTimeout || type == .tapDisabledByUserInput {
            discardPending(); gate.reset()
            if enabled, let tap { CGEvent.tapEnable(tap: tap, enable: true) }
            return Unmanaged.passUnretained(event)
        }
        guard enabled, type == .scrollWheel else { return Unmanaged.passUnretained(event) }
        // Pixel-continuous input (normally trackpads / smooth-scroll tools) passes unchanged.
        guard event.getIntegerValueField(.scrollWheelEventIsContinuous) == 0 else {
            continuousSeen += 1; discardPending(); gate.reset()
            return Unmanaged.passUnretained(event)
        }
        // Preserve horizontal or modified scrolling (e.g. zoom) without filtering.
        guard event.getIntegerValueField(.scrollWheelEventDeltaAxis2) == 0,
              event.flags.intersection([.maskShift, .maskControl, .maskAlternate, .maskCommand]).isEmpty else {
            discardPending(); gate.reset(); return Unmanaged.passUnretained(event)
        }
        let delta = event.getDoubleValueField(.scrollWheelEventFixedPtDeltaAxis1)
        let fallback = event.getIntegerValueField(.scrollWheelEventDeltaAxis1)
        let value = delta == 0 ? Double(fallback) : delta
        guard value != 0 else { return Unmanaged.passUnretained(event) }
        wheelSeen += 1
        let now = ProcessInfo.processInfo.systemUptime
        switch gate.process(sign: value > 0 ? 1 : -1, time: now) {
        case .pass:
            discardPending(); return Unmanaged.passUnretained(event)
        case .hold:
            if pending.count < 128, let copy = event.copy() { pending.append(copy) }
            else { dropped += 1 }
            return nil
        case .confirm:
            for buffered in pending { buffered.tapPostEvent(proxy) }
            pending.removeAll()
            return Unmanaged.passUnretained(event)
        }
    }
    func menuItem(_ title: String, _ action: Selector? = nil) -> NSMenuItem {
        let row = NSMenuItem(title: title, action: action, keyEquivalent: "")
        row.target = self; return row
    }
    func refreshMenu() {
        let active = enabled && tap != nil && AXIsProcessTrusted()
        item.button?.title = active ? "SWF ✓" : "SWF ○"
        let menu = NSMenu()
        menu.addItem(menuItem(statusText))
        menu.addItem(menuItem("Wheel: \(wheelSeen) · suppressed: \(dropped)"))
        menu.addItem(menuItem("Continuous input passed through: \(continuousSeen)"))
        menu.addItem(.separator())
        menu.addItem(menuItem(enabled ? "Pause filter" : "Enable filter", #selector(toggle)))
        let balanced = menuItem("Balanced — 80 ms", #selector(balancedMode))
        balanced.state = gate.window == 0.080 ? .on : .off; menu.addItem(balanced)
        let strong = menuItem("Strong — 140 ms", #selector(strongMode))
        strong.state = gate.window == 0.140 ? .on : .off; menu.addItem(strong)
        menu.addItem(menuItem("Setup and help…", #selector(showHelp(_:))))
        menu.addItem(.separator())
        let quit = menuItem("Quit Scroll Wheel Fix", #selector(quitApp)); quit.keyEquivalent = "q"
        menu.addItem(quit); item.menu = menu
        info?.stringValue = active ? "Filter active. Try scrolling with your mouse.\nUse the SWF menu to pause the filter or quit." : "Enable Accessibility for Scroll Wheel Fix in System Settings.\nThe app checks every two seconds and activates after permission is granted."
    }
    @objc func toggle() {
        enabled.toggle(); disconnect()
        statusText = enabled ? "Waiting for permission" : "Filter paused"
        connectIfAllowed(); refreshMenu()
    }
    @objc func balancedMode() { discardPending(); gate.reset(); gate.window = 0.080; refreshMenu() }
    @objc func strongMode() { discardPending(); gate.reset(); gate.window = 0.140; refreshMenu() }
    @objc func quitApp() { NSApp.terminate(nil) }
    @objc func openSettings() {
        let options = [kAXTrustedCheckOptionPrompt.takeUnretainedValue() as String: true] as CFDictionary
        _ = AXIsProcessTrustedWithOptions(options)
        NSWorkspace.shared.open(URL(string: "x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility")!)
    }
    @objc func showHelp(_ sender: Any?) {
        if window == nil {
            let w = NSWindow(contentRect: NSRect(x: 0, y: 0, width: 640, height: 430),
                styleMask: [.titled, .closable], backing: .buffered, defer: false)
            w.title = "Scroll Wheel Fix"; w.isReleasedWhenClosed = false; w.center()
            let title = NSTextField(labelWithString: "Scroll Wheel Fix")
            title.font = .boldSystemFont(ofSize: 23)
            let detail = NSTextField(wrappingLabelWithString: "Filters brief wrong-direction mouse-wheel bursts across mouse brands. This is a workaround, not a hardware repair. A deliberate reversal can wait for a follow-up tick; a single quick reverse tick may be suppressed.\n\nOnly scroll events are requested. Continuous input (usually trackpads), horizontal scroll, and modifier-key scrolling pass through. There is no network client or input log.")
            detail.font = .systemFont(ofSize: 14)
            let label = NSTextField(wrappingLabelWithString: "")
            label.font = .systemFont(ofSize: 14); info = label
            let button = NSButton(title: "Open Accessibility settings", target: self, action: #selector(openSettings))
            button.bezelStyle = .rounded
            let close = NSButton(title: "Quit and stop filtering", target: self, action: #selector(quitApp))
            close.bezelStyle = .rounded
            let stack = NSStackView(views: [title, detail, label, button, close])
            stack.orientation = .vertical; stack.alignment = .leading; stack.spacing = 18
            stack.translatesAutoresizingMaskIntoConstraints = false
            w.contentView!.addSubview(stack)
            NSLayoutConstraint.activate([
                stack.leadingAnchor.constraint(equalTo: w.contentView!.leadingAnchor, constant: 28),
                stack.trailingAnchor.constraint(equalTo: w.contentView!.trailingAnchor, constant: -28),
                stack.topAnchor.constraint(equalTo: w.contentView!.topAnchor, constant: 25),
                stack.bottomAnchor.constraint(lessThanOrEqualTo: w.contentView!.bottomAnchor, constant: -20)
            ])
            for field in [title, detail, label] { field.alignment = .left; field.baseWritingDirection = .leftToRight }
            window = w
        }
        refreshMenu(); window?.makeKeyAndOrderFront(nil); NSApp.activate(ignoringOtherApps: true)
    }
}

let app = NSApplication.shared
let delegate = ScrollFix()
app.delegate = delegate
app.run()
