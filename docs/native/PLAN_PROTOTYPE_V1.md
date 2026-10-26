# Prototype structurel — plan court
Autorisé par phase C de la mission, sous recommandation H. Prototype de recherche séparé ; aucun raccordement AUv3 tant que L01 n’est pas clos.
1. Écrire Tests/test_engine.cpp pour ordre, origine DOWN/UP, séparation TX/observation, BAR/STOP, deux destinations et refus bornés.
2. Ajouter Core/Engine.hpp (C++17, stockage fixe, aucun iOS, aucune adresse MIDI musicale nouvelle). L’API consomme Event ordonnés et produit Command logiques.
3. Vérifier la suite sur CI Ubuntu/macOS, avec warnings stricts et sanitizers ; exécuter aussi Tests/run.sh existant.
4. Publier seulement branche dédiée. Documenter limites : aucun calcul de BAR AUM, geste complet, persistance, profil réel, saturation physique ou preuve musicale.
L02 reste conditionné par la matrice fonctionnelle qualifiée. Les capacités du prototype ne sont pas les budgets V1.
