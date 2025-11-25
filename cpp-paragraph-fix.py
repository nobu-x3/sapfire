#!/usr/bin/env python3
"""
Remove empty lines inside C++ functions/methods.
Makes each function a single paragraph for vim/neovim navigation.
"""

import sys
import argparse
from pathlib import Path

def remove_empty_lines_in_functions(content):
    """Remove empty lines inside C++ functions."""
    lines = content.split('\n')
    result = []

    # Simple state tracking
    brace_depth = 0

    for i, line in enumerate(lines):
        # Count braces, but skip those in comments and strings
        # This is a simplified approach but works for most code
        temp_line = line

        # Remove string contents (basic)
        while '"' in temp_line:
            start = temp_line.find('"')
            end = temp_line.find('"', start + 1)
            if end == -1:
                break
            temp_line = temp_line[:start] + temp_line[end+1:]

        # Remove char contents (basic)
        while "'" in temp_line:
            start = temp_line.find("'")
            end = temp_line.find("'", start + 1)
            if end == -1 or end - start > 3:  # Not a char literal
                break
            temp_line = temp_line[:start] + temp_line[end+1:]

        # Remove single-line comments
        if '//' in temp_line:
            temp_line = temp_line[:temp_line.find('//')]

        # Count braces
        open_braces = temp_line.count('{')
        close_braces = temp_line.count('}')

        old_depth = brace_depth
        brace_depth += open_braces - close_braces

        # Determine if this line should be kept
        stripped = line.strip()
        is_empty = len(stripped) == 0

        # Simple heuristic: if we're at depth > 0, we're inside something (function, class, etc.)
        # Only remove empty lines when we're at depth > 0
        # This isn't perfect but works for most cases and is safe
        if is_empty and brace_depth > 0:
            # Don't remove if this is between class members (heuristic)
            # Look ahead to see if next non-empty line looks like a member declaration
            next_non_empty = None
            for j in range(i + 1, min(i + 5, len(lines))):
                if lines[j].strip():
                    next_non_empty = lines[j].strip()
                    break

            # Keep empty lines before access specifiers or between class methods
            if next_non_empty and any(spec in next_non_empty for spec in
                ['public:', 'private:', 'protected:', 'virtual ', 'static ', 'void ', 'int ', 'bool ',
                 'double ', 'float ', 'char ', 'auto ', 'template']):
                # Check if we might be between class members
                if old_depth == 1:  # Likely in class scope
                    result.append(line)
                    continue

            # Otherwise skip the empty line (it's inside a function)
            continue
        else:
            # Keep the line
            result.append(line)

    return '\n'.join(result)

def process_file(filepath, dry_run=False, verbose=False):
    """Process a single C++ file."""
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            original = f.read()

        modified = remove_empty_lines_in_functions(original)

        if original != modified:
            removed = original.count('\n') - modified.count('\n')

            if verbose or dry_run:
                print(f"{'Would remove' if dry_run else 'Removing'} {removed} empty lines from: {filepath}")

            if not dry_run:
                with open(filepath, 'w', encoding='utf-8') as f:
                    f.write(modified)

            return True
        else:
            if verbose:
                print(f"No changes needed: {filepath}")
            return False

    except Exception as e:
        print(f"Error processing {filepath}: {e}", file=sys.stderr)
        return False

def main():
    parser = argparse.ArgumentParser(
        description='Remove empty lines inside C++ functions for vim/neovim paragraph navigation'
    )
    parser.add_argument('path', help='File or directory to process')
    parser.add_argument('-r', '--recursive', action='store_true',
                       help='Process directories recursively')
    parser.add_argument('-e', '--extensions', default='cpp,cc,cxx,c,h,hpp,hxx',
                       help='File extensions (default: cpp,cc,cxx,c,h,hpp,hxx)')
    parser.add_argument('-n', '--dry-run', action='store_true',
                       help='Preview changes without modifying files')
    parser.add_argument('-v', '--verbose', action='store_true',
                       help='Verbose output')

    args = parser.parse_args()

    path = Path(args.path)
    extensions = args.extensions.split(',')
    modified_count = 0
    total_files = 0

    if path.is_file():
        total_files = 1
        if process_file(path, args.dry_run, args.verbose):
            modified_count += 1
    elif path.is_dir():
        files = []
        pattern = '**/*' if args.recursive else '*'
        for ext in extensions:
            files.extend(path.glob(f"{pattern}.{ext}"))

        files = [f for f in files if f.is_file()]
        total_files = len(files)

        if total_files == 0:
            print(f"No C++ files found in {path}")
            return

        for file in files:
            if process_file(file, args.dry_run, args.verbose):
                modified_count += 1
    else:
        print(f"Error: {path} not found", file=sys.stderr)
        sys.exit(1)

    # Summary
    print(f"\nProcessed {total_files} file(s), modified {modified_count}")
    if args.dry_run and modified_count > 0:
        print("Run without --dry-run to apply changes")

if __name__ == '__main__':
    main()
