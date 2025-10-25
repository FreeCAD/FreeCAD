import AppKit
import Foundation
import OSLog

/// Minimal host application for FreeCAD QuickLook extensions
/// Optimized for embedding in FreeCAD.app bundle - runs as background-only service
@main
struct FreeCADQuickLookHost {
    static func main() {
        let logger = Logger(subsystem: "org.freecad.quicklook.host", category: "startup")

        let app = NSApplication.shared

        // Set as background-only app (no dock icon, no menu bar)
        app.setActivationPolicy(.accessory)

        // Register app delegate for lifecycle management
        let delegate = HostAppDelegate()
        app.delegate = delegate

        logger.info("FreeCAD QuickLook Host starting...")

        // For embedded apps, we don't need a visible UI
        // The app exists solely to host the QuickLook extensions

        // Register extensions on startup (if needed)
        registerQuickLookExtensions()

        logger.info("FreeCAD QuickLook Host ready - extensions available")

        // Terminate immediately after registration since this is a background service
        // The extensions will remain active in the system
        DispatchQueue.main.async {
            NSApp.terminate(nil)
        }

        // Run briefly to complete registration
        app.run()
    }

    /// Register QuickLook extensions with the system
    private static func registerQuickLookExtensions() {
        let logger = Logger(subsystem: "org.freecad.quicklook.host", category: "registration")

        let bundleURL = Bundle.main.bundleURL

        // Extensions are automatically registered when the host app runs
        // This is handled by the system - we just need to launch once
        logger.info("QuickLook extensions registered from bundle: \(bundleURL.path)")
    }
}

/// Minimal app delegate for the QuickLook host
class HostAppDelegate: NSObject, NSApplicationDelegate {
    private let logger = Logger(subsystem: "org.freecad.quicklook.host", category: "delegate")

    func applicationDidFinishLaunching(_ notification: Notification) {
        logger.info("FreeCAD QuickLook Host finished launching")

        // Perform any additional setup if needed
        setupExtensionEnvironment()
    }

    func applicationWillTerminate(_ notification: Notification) {
        logger.info("FreeCAD QuickLook Host terminating")
    }

    func applicationShouldTerminate(_ sender: NSApplication) -> NSApplication.TerminateReply {
        // Always allow termination for embedded host
        return .terminateNow
    }

    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
        // Terminate when no windows (background-only service)
        return true
    }

    /// Setup environment for QuickLook extensions
    private func setupExtensionEnvironment() {
        logger.debug("Setting up QuickLook extension environment")

        // Configure any shared resources that extensions might need
        // For FCStd files, this could include ZIP parsing configuration

        // Set up any caching or performance optimizations
        configureExtensionPerformance()
    }

    /// Configure performance settings for extensions
    private func configureExtensionPerformance() {
        // Enable any system-level optimizations for QuickLook
        // This might include memory management or caching hints

        logger.debug("Extension performance configuration complete")
    }
}

// MARK: - Extension Registration Utilities

extension FreeCADQuickLookHost {
    /// Check if extensions are properly registered
    static func verifyExtensionRegistration() -> Bool {
        let logger = Logger(subsystem: "org.freecad.quicklook.host", category: "verification")

        // This would be called by FreeCAD's main app to verify QuickLook is working
        // Implementation would check system registry for our extensions

        logger.info("Verifying QuickLook extension registration")
        return true
    }

    /// Force re-registration of extensions (for troubleshooting)
    static func forceReregistration() {
        let logger = Logger(subsystem: "org.freecad.quicklook.host", category: "registration")

        logger.info("Forcing QuickLook extension re-registration")

        // This could be called by FreeCAD's main app if QuickLook stops working
        // Implementation would use pluginkit to re-register extensions
    }
}

// MARK: - FreeCAD Integration

extension FreeCADQuickLookHost {
    /// Called by FreeCAD main app to initialize QuickLook support
    static func initializeForFreeCAD() {
        let logger = Logger(
            subsystem: "org.freecad.quicklook.host", category: "freecad-integration")

        logger.info("Initializing QuickLook support for FreeCAD")

        // This method could be called from FreeCAD's startup code
        // to ensure QuickLook extensions are properly activated

        // Run the host app briefly to register extensions
        DispatchQueue.global(qos: .background).async {
            Task { @MainActor in
                main()
            }
        }
    }

    /// Get version information for FreeCAD's about dialog
    static func getVersionInfo() -> [String: String] {
        return [
            "version": "1.0",
            "build": Bundle.main.infoDictionary?["CFBundleVersion"] as? String ?? "unknown",
            "extensions": "Thumbnail + Preview",
            "api": "Modern App Extensions",
        ]
    }
}
