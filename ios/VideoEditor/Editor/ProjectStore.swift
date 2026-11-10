import Foundation

/// Where the working project file lives.
///
/// There is one working project for now: reopening the app puts the user back
/// on the timeline they left, which is what a phone editor should do. Named
/// projects and a project browser come with the multi-project phase.
enum ProjectStore {
    static var directory: URL {
        let base = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0]
        return base.appendingPathComponent("Projects", isDirectory: true)
    }

    static var currentProjectURL: URL {
        directory.appendingPathComponent("current.vxproj")
    }

    static func ensureDirectory() throws {
        try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
    }
}
