import os
import subprocess
import sys
import difflib
import argparse

def get_files(directories, extensions):
    """Finds all files with matching extensions in the given directories."""
    files_found = []
    for d in directories:
        if os.path.exists(d):
            for root, _, files in os.walk(d):
                for file in files:
                    if file.endswith(extensions):
                        files_found.append(os.path.join(root, file))
    return files_found

def check_files(files):
    """Runs clang-format in dry-run mode and prints a unified diff."""
    print(f"Checking {len(files)} files for formatting violations...")
    found_diff = False

    for filepath in files:
        try:
            # Read original content
            with open(filepath, 'r', encoding='utf-8') as f:
                original = f.readlines()

            # Run clang-format and capture output (no -i)
            # We use check=True to ensure clang-format ran successfully
            result = subprocess.run(
                ['clang-format', filepath],
                capture_output=True,
                text=True,
                check=True
            )
            
            formatted = result.stdout.splitlines(keepends=True)

            # Generate diff
            diff = list(difflib.unified_diff(
                original, 
                formatted, 
                fromfile=f'a/{filepath}', 
                tofile=f'b/{filepath}',
                lineterm=''
            ))

            if diff:
                found_diff = True
                print(f"\n[!] Formatting changes needed for {filepath}:")
                sys.stdout.writelines(diff)

        except subprocess.CalledProcessError as e:
            print(f"Error running clang-format on {filepath}: {e}")
        except UnicodeDecodeError:
            print(f"Skipping binary or non-utf8 file: {filepath}")

    if found_diff:
        print("\nFormatting issues found.")
        sys.exit(1)
    else:
        print("No formatting changes needed.")
        sys.exit(0)

def format_in_place(files):
    """Runs clang-format -i on all files."""
    if not files:
        print("No files to format.")
        return

    print(f"Formatting {len(files)} files...")
    # 'clang-format -i' allows multiple files at once, which is faster
    cmd = ['clang-format', '-i'] + files
    subprocess.run(cmd, check=True)
    print("Done.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Format or check C/C++ project files.")
    parser.add_argument('root', nargs='?', default='.', help="Project root directory")
    parser.add_argument('--check', action='store_true', help="Show diffs instead of editing files")
    
    args = parser.parse_args()
    project_root = args.root

    # Define your folders and extensions here
    dirs = [
        os.path.join(project_root, 'src'), 
        os.path.join(project_root, 'include')
    ]
    exts = ('.c', '.h') 
    
    files = get_files(dirs, exts)

    if args.check:
        check_files(files)
    else:
        format_in_place(files)
