#!/bin/bash

# [B] / [BRANCH] - Submodule-first push script

# 1. Primeiro lida com os submodules
git submodule foreach '
    if [ -n "$(git status --porcelain)" ]; then
        echo "-------------------------------------------------------"
        echo "Alterações detectadas no submodule: $name"
        read -p "Mensagem de commit para \"$name\": " sub_msg
        git add .
        git commit -m "$sub_msg"
        git push
    else
        echo "Submodule $name sem alterações. Pulando..."
    fi
'

# 2. Depois lida com o repositório principal (Root)
echo "-------------------------------------------------------"
echo "Finalizando com o repositório principal..."

if [ -n "$(git status --porcelain)" ]; then
    read -p "Mensagem de commit (Main/Root): " msg
    git add .
    git commit -m "$msg"
    git push
else
    echo "Nada para alterar no root."
fi