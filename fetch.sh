echo "git fetch"
git fetch --all

echo "--------------------------------------------------------------------------------"
echo "git status"
git status

echo "--------------------------------------------------------------------------------"
echo "git log last 2 days"
git log --all --oneline --decorate --graph --since="2 days"
