# Fixture de compatibilité V2

`legacy_pair_patch_v2.bin` a été produit avec les headers Core/CoreV3/Shared archivés au commit 2638f2293f6f244b86e8dd5e933bbee306942409, avant les exclusions MACHINE paires.

Session schema1/config1/revision1, PATCH source0/slot0 enregistré via HostEngine::saveSourcePatch : 60 valeurs (continues44, états1), autres cellules UNKNOWN. Document publié par beginRenderBuffer/endRenderBuffer puis encodeDocument. Aucun objet mémoire brut ni donnée matérielle.

Taille : 79388 octets. SHA256 : 94202810ce2a7194886e34aeca7d2d583bf8fb10c4c67428da2978bc2cf302eb.

Le test vérifie que les anciennes valeurs centrales restent décodées, mais ne sont pas rejouées vers les FX INST. Il ne prétend pas que l'ancien binaire sait lire un nouveau SAVE contenant les exclusions UNKNOWN.
