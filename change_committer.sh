#!/bin/bash

# Script to change author and committer for the last N commits

set -e  # Exit on error

# Check arguments
if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <number_of_commits> <author_name> <author_email>"
    echo "Example: $0 5 \"John Doe\" \"john@example.com\""
    exit 1
fi

N=$1
AUTHOR_NAME=$2
AUTHOR_EMAIL=$3

# Validate N is a positive number
if ! [[ "$N" =~ ^[0-9]+$ ]] || [ "$N" -le 0 ]; then
    echo "Error: Number of commits must be a positive integer"
    exit 1
fi

# Check if we're in a git repository
if ! git rev-parse --git-dir > /dev/null 2>&1; then
    echo "Error: Not in a git repository"
    exit 1
fi

# Check if there are uncommitted changes
if ! git diff-index --quiet HEAD --; then
    echo "Error: You have uncommitted changes. Please commit or stash them first."
    exit 1
fi

# Check if we have enough commits
TOTAL_COMMITS=$(git rev-list --count HEAD)
if [ "$N" -gt "$TOTAL_COMMITS" ]; then
    echo "Error: Repository only has $TOTAL_COMMITS commits, but you requested to change $N"
    exit 1
fi

# Show what will be changed
echo "========================================="
echo "This will change the last $N commit(s)"
echo "New Author/Committer: $AUTHOR_NAME <$AUTHOR_EMAIL>"
echo "========================================="
echo ""
echo "Commits that will be modified:"
git log --oneline -n "$N"
echo ""
echo "WARNING: This rewrites git history!"
echo "If these commits have been pushed, you'll need to force push."
echo ""
read -p "Do you want to continue? (yes/no): " CONFIRM

if [ "$CONFIRM" != "yes" ]; then
    echo "Aborted."
    exit 0
fi

# Perform the rebase using filter-branch
echo ""
echo "Rewriting commits..."

git filter-branch -f --env-filter "
    export GIT_AUTHOR_NAME='$AUTHOR_NAME'
    export GIT_AUTHOR_EMAIL='$AUTHOR_EMAIL'
    export GIT_COMMITTER_NAME='$AUTHOR_NAME'
    export GIT_COMMITTER_EMAIL='$AUTHOR_EMAIL'
" HEAD~"$N"..HEAD

echo ""
echo "✓ Successfully changed author and committer for the last $N commit(s)"
echo ""
echo "To push these changes (if already pushed to remote):"
echo "  git push --force-with-lease"
