#ifndef SERIAL_FILE_HANDLER_H
#define SERIAL_FILE_HANDLER_H

#include <Arduino.h>
#include <FS.h>
#include <FFat.h>

// Current working directory (starts at root)
String currentDir = "/";

void printFileHelp() {
    Serial.println("=========== File System Commands ===========");
    Serial.println("  file ls [path]           - List directory contents");
    Serial.println("  file dir [path]          - List directory contents (alias)");
    Serial.println("  file cd <path>           - Change directory");
    Serial.println("  file pwd                 - Print working directory");
    Serial.println("  file mkdir <path>        - Create directory");
    Serial.println("  file rmdir <path>        - Remove directory");
    Serial.println("  file cat <file>          - Display file contents");
    Serial.println("  file rm <file>           - Remove file");
    Serial.println("  file touch <file>        - Create empty file");
    Serial.println("  file mv <src> <dst>      - Rename/move file");
    Serial.println("  file cp <src> <dst>      - Copy file");
    Serial.println("  file storage             - Show storage information");
    Serial.println("  file du [path]           - Disk usage");
    Serial.println("  file tree [path]         - Tree view of directory");
    Serial.println("  file format              - Format filesystem (WARNING: erases all data!)");
    Serial.println("Examples:");
    Serial.println("  file ls /");
    Serial.println("  file cat /config.json");
    Serial.println("  file mkdir /logs");
}

// Resolve relative paths to absolute paths
String resolvePath(String path) {
    if (path.length() == 0) {
        return currentDir;
    }
    
    // Already absolute
    if (path.startsWith("/")) {
        return path;
    }
    
    // Relative path
    String resolved = currentDir;
    if (!resolved.endsWith("/")) {
        resolved += "/";
    }
    resolved += path;
    
    // Remove trailing slash unless it's root
    if (resolved.length() > 1 && resolved.endsWith("/")) {
        resolved.remove(resolved.length() - 1);
    }
    
    return resolved;
}

// Format file size
String formatSize(size_t bytes) {
    if (bytes < 1024) {
        return String(bytes) + " B";
    } else if (bytes < 1024 * 1024) {
        return String(bytes / 1024.0, 1) + " KB";
    } else {
        return String(bytes / (1024.0 * 1024.0), 1) + " MB";
    }
}

// List directory contents
void cmdListDir(String path) {
    path = resolvePath(path);
    
    File dir = FFat.open(path);
    if (!dir) {
        Serial.printf("[File] ✗ Cannot open: %s\n", path.c_str());
        return;
    }
    
    if (!dir.isDirectory()) {
        Serial.printf("[File] ✗ Not a directory: %s\n", path.c_str());
        dir.close();
        return;
    }
    
    Serial.printf("\n=== Directory: %s ===\n\n", path.c_str());
    Serial.println("Type    Size          Name");
    Serial.println("------  ------------  ----------------------------------");
    
    int fileCount = 0, dirCount = 0;
    size_t totalSize = 0;
    
    File file = dir.openNextFile();
    while (file) {
        String name = String(file.name());
        // Remove path prefix to show only filename
        int lastSlash = name.lastIndexOf('/');
        if (lastSlash >= 0) {
            name = name.substring(lastSlash + 1);
        }
        
        if (file.isDirectory()) {
            Serial.printf("[DIR]   %-12s  %s/\n", "-", name.c_str());
            dirCount++;
        } else {
            Serial.printf("[FILE]  %-12s  %s\n", formatSize(file.size()).c_str(), name.c_str());
            totalSize += file.size();
            fileCount++;
        }
        
        file = dir.openNextFile();
    }
    
    dir.close();
    
    Serial.println("------  ------------  ----------------------------------");
    Serial.printf("Total: %d files (%s), %d directories\n\n", 
                  fileCount, formatSize(totalSize).c_str(), dirCount);
}

// Change directory
void cmdChangeDir(String path) {
    path = resolvePath(path);
    
    // Handle special cases
    if (path == "..") {
        // Go to parent directory
        int lastSlash = currentDir.lastIndexOf('/');
        if (lastSlash > 0) {
            currentDir = currentDir.substring(0, lastSlash);
        } else {
            currentDir = "/";
        }
        Serial.printf("[File] ✓ Directory: %s\n", currentDir.c_str());
        return;
    }
    
    File dir = FFat.open(path);
    if (!dir) {
        Serial.printf("[File] ✗ Directory not found: %s\n", path.c_str());
        return;
    }
    
    if (!dir.isDirectory()) {
        Serial.printf("[File] ✗ Not a directory: %s\n", path.c_str());
        dir.close();
        return;
    }
    
    dir.close();
    currentDir = path;
    Serial.printf("[File] ✓ Directory: %s\n", currentDir.c_str());
}

// Print working directory
void cmdPrintWorkingDir() {
    Serial.printf("Current directory: %s\n", currentDir.c_str());
}

// Create directory
void cmdMakeDir(String path) {
    path = resolvePath(path);
    
    if (FFat.exists(path)) {
        Serial.printf("[File] ✗ Already exists: %s\n", path.c_str());
        return;
    }
    
    if (FFat.mkdir(path)) {
        Serial.printf("[File] ✓ Directory created: %s\n", path.c_str());
    } else {
        Serial.printf("[File] ✗ Failed to create: %s\n", path.c_str());
    }
}

// Remove directory
void cmdRemoveDir(String path) {
    path = resolvePath(path);
    
    if (!FFat.exists(path)) {
        Serial.printf("[File] ✗ Not found: %s\n", path.c_str());
        return;
    }
    
    File dir = FFat.open(path);
    if (!dir.isDirectory()) {
        Serial.printf("[File] ✗ Not a directory: %s\n", path.c_str());
        dir.close();
        return;
    }
    
    // Check if directory is empty
    File file = dir.openNextFile();
    if (file) {
        Serial.printf("[File] ✗ Directory not empty: %s\n", path.c_str());
        Serial.println("[File] Use 'rm -r' to remove recursively");
        file.close();
        dir.close();
        return;
    }
    dir.close();
    
    if (FFat.rmdir(path)) {
        Serial.printf("[File] ✓ Directory removed: %s\n", path.c_str());
    } else {
        Serial.printf("[File] ✗ Failed to remove: %s\n", path.c_str());
    }
}

// Display file contents
void cmdCat(String path) {
    path = resolvePath(path);
    
    File file = FFat.open(path, "r");
    if (!file) {
        Serial.printf("[File] ✗ Cannot open: %s\n", path.c_str());
        return;
    }
    
    if (file.isDirectory()) {
        Serial.printf("[File] ✗ Is a directory: %s\n", path.c_str());
        file.close();
        return;
    }
    
    Serial.printf("\n=== File: %s (%s) ===\n\n", path.c_str(), formatSize(file.size()).c_str());
    
    while (file.available()) {
        Serial.write(file.read());
    }
    
    Serial.println("\n");
    file.close();
}

// Remove file
void cmdRemoveFile(String path, bool recursive = false) {
    path = resolvePath(path);
    
    if (!FFat.exists(path)) {
        Serial.printf("[File] ✗ Not found: %s\n", path.c_str());
        return;
    }
    
    File file = FFat.open(path);
    bool isDir = file.isDirectory();
    file.close();
    
    if (isDir && !recursive) {
        Serial.printf("[File] ✗ Is a directory: %s\n", path.c_str());
        Serial.println("[File] Use 'rm -r' to remove directories");
        return;
    }
    
    if (FFat.remove(path)) {
        Serial.printf("[File] ✓ Removed: %s\n", path.c_str());
    } else {
        Serial.printf("[File] ✗ Failed to remove: %s\n", path.c_str());
    }
}

// Create empty file
void cmdTouch(String path) {
    path = resolvePath(path);
    
    if (FFat.exists(path)) {
        Serial.printf("[File] ✗ Already exists: %s\n", path.c_str());
        return;
    }
    
    File file = FFat.open(path, "w");
    if (file) {
        file.close();
        Serial.printf("[File] ✓ File created: %s\n", path.c_str());
    } else {
        Serial.printf("[File] ✗ Failed to create: %s\n", path.c_str());
    }
}

// Copy file
void cmdCopyFile(String src, String dst) {
    src = resolvePath(src);
    dst = resolvePath(dst);
    
    File srcFile = FFat.open(src, "r");
    if (!srcFile) {
        Serial.printf("[File] ✗ Source not found: %s\n", src.c_str());
        return;
    }
    
    if (srcFile.isDirectory()) {
        Serial.printf("[File] ✗ Source is directory: %s\n", src.c_str());
        srcFile.close();
        return;
    }
    
    if (FFat.exists(dst)) {
        Serial.printf("[File] ✗ Destination exists: %s\n", dst.c_str());
        srcFile.close();
        return;
    }
    
    File dstFile = FFat.open(dst, "w");
    if (!dstFile) {
        Serial.printf("[File] ✗ Cannot create: %s\n", dst.c_str());
        srcFile.close();
        return;
    }
    
    Serial.printf("[File] Copying %s...\n", formatSize(srcFile.size()).c_str());
    
    uint8_t buffer[512];
    while (srcFile.available()) {
        size_t bytesRead = srcFile.read(buffer, sizeof(buffer));
        dstFile.write(buffer, bytesRead);
    }
    
    srcFile.close();
    dstFile.close();
    
    Serial.printf("[File] ✓ Copied: %s -> %s\n", src.c_str(), dst.c_str());
}

// Rename/move file
void cmdMoveFile(String src, String dst) {
    src = resolvePath(src);
    dst = resolvePath(dst);
    
    if (!FFat.exists(src)) {
        Serial.printf("[File] ✗ Source not found: %s\n", src.c_str());
        return;
    }
    
    if (FFat.exists(dst)) {
        Serial.printf("[File] ✗ Destination exists: %s\n", dst.c_str());
        return;
    }
    
    if (FFat.rename(src, dst)) {
        Serial.printf("[File] ✓ Moved: %s -> %s\n", src.c_str(), dst.c_str());
    } else {
        Serial.printf("[File] ✗ Failed to move: %s\n", src.c_str());
    }
}

// Show storage information
void cmdStorage() {
    Serial.println("\n=== Storage Information ===");
    Serial.printf("Total Space:  %s\n", formatSize(FFat.totalBytes()).c_str());
    Serial.printf("Used Space:   %s\n", formatSize(FFat.usedBytes()).c_str());
    Serial.printf("Free Space:   %s\n", formatSize(FFat.totalBytes() - FFat.usedBytes()).c_str());
    
    float usage = (float)FFat.usedBytes() / (float)FFat.totalBytes() * 100.0;
    Serial.printf("Usage:        %.1f%%\n", usage);
    Serial.println("===========================\n");
}

// Format filesystem
void cmdFormat() {
    Serial.println("\n========================================");
    Serial.println("   ⚠️  WARNING: FORMAT FILESYSTEM  ⚠️");
    Serial.println("========================================");
    Serial.println("This will ERASE ALL DATA on the filesystem!");
    Serial.println("Type 'YES' to confirm format (30 second timeout):");
    Serial.print("> ");
    
    // Wait for confirmation with timeout
    unsigned long startTime = millis();
    String confirmation = "";
    
    while (millis() - startTime < 30000) {  // 30 second timeout
        if (Serial.available()) {
            char c = Serial.read();
            if (c == '\n' || c == '\r') {
                Serial.println();
                break;
            }
            Serial.print(c);
            confirmation += c;
        }
    }
    
    confirmation.trim();
    confirmation.toUpperCase();
    
    if (confirmation != "YES") {
        Serial.println("\n[File] ✗ Format cancelled");
        return;
    }
    
    Serial.println("\n[File] Formatting filesystem...");
    
    // Unmount first
    FFat.end();
    delay(100);
    
    // Format
    if (!FFat.format()) {
        Serial.println("[File] ✗ Format failed!");
        // Try to remount anyway
        FFat.begin();
        return;
    }
    
    Serial.println("[File] ✓ Format complete");
    
    // Remount
    if (FFat.begin()) {
        Serial.println("[File] ✓ Filesystem remounted");
        currentDir = "/";  // Reset to root
        cmdStorage();  // Show new storage info
    } else {
        Serial.println("[File] ✗ Failed to remount filesystem");
    }
}



// Calculate disk usage for directory
size_t calculateDirSize(String path) {
    File dir = FFat.open(path);
    if (!dir || !dir.isDirectory()) {
        return 0;
    }
    
    size_t totalSize = 0;
    File file = dir.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            totalSize += file.size();
        }
        file = dir.openNextFile();
    }
    dir.close();
    
    return totalSize;
}

// Disk usage command
void cmdDiskUsage(String path) {
    path = resolvePath(path);
    
    File dir = FFat.open(path);
    if (!dir) {
        Serial.printf("[File] ✗ Cannot open: %s\n", path.c_str());
        return;
    }
    
    if (!dir.isDirectory()) {
        Serial.printf("[File] Size: %s\n", formatSize(dir.size()).c_str());
        dir.close();
        return;
    }
    
    Serial.printf("\n=== Disk Usage: %s ===\n\n", path.c_str());
    
    size_t totalSize = 0;
    File file = dir.openNextFile();
    while (file) {
        String name = String(file.name());
        int lastSlash = name.lastIndexOf('/');
        if (lastSlash >= 0) {
            name = name.substring(lastSlash + 1);
        }
        
        if (file.isDirectory()) {
            Serial.printf("%-12s  %s/\n", "-", name.c_str());
        } else {
            Serial.printf("%-12s  %s\n", formatSize(file.size()).c_str(), name.c_str());
            totalSize += file.size();
        }
        
        file = dir.openNextFile();
    }
    dir.close();
    
    Serial.println("------------------------");
    Serial.printf("Total: %s\n\n", formatSize(totalSize).c_str());
}

// Tree view helper (recursive)
void printTreeHelper(String path, String prefix, bool isLast) {
    File dir = FFat.open(path);
    if (!dir || !dir.isDirectory()) {
        return;
    }
    
    File file = dir.openNextFile();
    File nextFile = dir.openNextFile();
    
    dir.close();
    dir = FFat.open(path);
    dir.openNextFile(); // Skip first to restart
    
    file = dir.openNextFile();
    while (file) {
        String name = String(file.name());
        int lastSlash = name.lastIndexOf('/');
        if (lastSlash >= 0) {
            name = name.substring(lastSlash + 1);
        }
        
        // Check if this is the last file
        File peek = dir.openNextFile();
        bool last = (peek.name()[0] == 0);
        if (peek) peek.close();
        
        Serial.print(prefix);
        Serial.print(last ? "└── " : "├── ");
        Serial.println(name + (file.isDirectory() ? "/" : ""));
        
        if (file.isDirectory()) {
            String fullPath = String(file.path());
            printTreeHelper(fullPath, prefix + (last ? "    " : "│   "), last);
        }
        
        file = dir.openNextFile();
    }
    dir.close();
}

// Tree view command
void cmdTree(String path) {
    path = resolvePath(path);
    
    File dir = FFat.open(path);
    if (!dir) {
        Serial.printf("[File] ✗ Cannot open: %s\n", path.c_str());
        return;
    }
    
    if (!dir.isDirectory()) {
        Serial.printf("[File] ✗ Not a directory: %s\n", path.c_str());
        dir.close();
        return;
    }
    dir.close();
    
    Serial.printf("\n%s\n", path.c_str());
    printTreeHelper(path, "", true);
    Serial.println();
}

// Main file command handler
void handleFileCommand(String args) {
    args.trim();
    
    // Parse subcommand
    int spaceIndex = args.indexOf(' ');
    String subCmd, subArgs;
    
    if (spaceIndex > 0) {
        subCmd = args.substring(0, spaceIndex);
        subArgs = args.substring(spaceIndex + 1);
        subArgs.trim();
    } else {
        subCmd = args;
        subArgs = "";
    }
    
    subCmd.toLowerCase();
    
    if (subCmd == "" || subCmd == "help" || subCmd == "?") {
        printFileHelp();
    }
    else if (subCmd == "ls" || subCmd == "dir") {
        cmdListDir(subArgs.length() > 0 ? subArgs : currentDir);
    }
    else if (subCmd == "cd") {
        if (subArgs.length() == 0) {
            currentDir = "/";
            Serial.println("[File] ✓ Directory: /");
        } else {
            cmdChangeDir(subArgs);
        }
    }
    else if (subCmd == "pwd") {
        cmdPrintWorkingDir();
    }
    else if (subCmd == "mkdir") {
        if (subArgs.length() == 0) {
            Serial.println("[File] ✗ Error: Directory name required");
            Serial.println("Usage: file mkdir <path>");
            return;
        }
        cmdMakeDir(subArgs);
    }
    else if (subCmd == "rmdir") {
        if (subArgs.length() == 0) {
            Serial.println("[File] ✗ Error: Directory name required");
            Serial.println("Usage: file rmdir <path>");
            return;
        }
        cmdRemoveDir(subArgs);
    }
    else if (subCmd == "cat") {
        if (subArgs.length() == 0) {
            Serial.println("[File] ✗ Error: File name required");
            Serial.println("Usage: file cat <file>");
            return;
        }
        cmdCat(subArgs);
    }
    else if (subCmd == "rm") {
        if (subArgs.length() == 0) {
            Serial.println("[File] ✗ Error: File name required");
            Serial.println("Usage: file rm <file>");
            return;
        }
        
        // Check for -r flag
        bool recursive = false;
        if (subArgs.startsWith("-r ")) {
            recursive = true;
            subArgs = subArgs.substring(3);
            subArgs.trim();
        }
        
        cmdRemoveFile(subArgs, recursive);
    }
    else if (subCmd == "touch") {
        if (subArgs.length() == 0) {
            Serial.println("[File] ✗ Error: File name required");
            Serial.println("Usage: file touch <file>");
            return;
        }
        cmdTouch(subArgs);
    }
    else if (subCmd == "cp") {
        // Parse source and destination
        int space = subArgs.indexOf(' ');
        if (space <= 0) {
            Serial.println("[File] ✗ Error: Source and destination required");
            Serial.println("Usage: file cp <source> <destination>");
            return;
        }
        
        String src = subArgs.substring(0, space);
        String dst = subArgs.substring(space + 1);
        dst.trim();
        
        cmdCopyFile(src, dst);
    }
    else if (subCmd == "mv") {
        // Parse source and destination
        int space = subArgs.indexOf(' ');
        if (space <= 0) {
            Serial.println("[File] ✗ Error: Source and destination required");
            Serial.println("Usage: file mv <source> <destination>");
            return;
        }
        
        String src = subArgs.substring(0, space);
        String dst = subArgs.substring(space + 1);
        dst.trim();
        
        cmdMoveFile(src, dst);
    }
    else if (subCmd == "storage") {
        cmdStorage();
    }
    else if (subCmd == "du") {
        cmdDiskUsage(subArgs.length() > 0 ? subArgs : currentDir);
    }
    else if (subCmd == "tree") {
        cmdTree(subArgs.length() > 0 ? subArgs : currentDir);
    }
    else if (subCmd == "format") {
        cmdFormat();
    }
    else {
        Serial.printf("[File] ✗ Unknown command: %s\n", subCmd.c_str());
        Serial.println("[File] Type 'file help' for available commands");
    }
}

#endif // SERIAL_FILE_HANDLER_H
