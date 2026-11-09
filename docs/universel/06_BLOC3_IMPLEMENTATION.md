# Bloc 3 — snapshots globaux natifs — 28 octobre 2026

## Réalisé
Quatre banques PRESET natives, initialement vides. Sur la face PRESET qualifiée, les quatre boutons36..39 capturent au premier engagement puis demandent le rappel au BAR. INIT32 reste une commande distincte au BEAT, sans cinquième banque ; sa référence locale doit être explicitement renseignée, sinon refus sans RESET inventé. La face s'ouvre par le paramètre AU natif `PRESET editor` (adresse AU0, espace distinct de MIDI). L'intention passe atomiquement vers le thread de rendu ; S3 reste seul écrivain NAV. Les événements d'automation sont admis au sample fourni. Aucun nouveau combo K2 ne remplace BOT2/MUTE.

RED physique BOT1/note12 : premier DOWN capture le monde courant ; premier UP court conserve RED actif ; second appui court ou UP long restaure immédiatement. STOP retire son autorité sans restauration et son ancien UP reste inerte. SAVE PATCH est refusé pendant RED, sans rappel de secours.

Les images musicales contiennent valeurs connues ou UNKNOWN, NAV aplati, états appliqués MUTE/SOLO, SOFT, sélections CUE/LOAD désirées et engagements MEMORY/PRESET. Elles ne contiennent ni gestes, ni retours LONG, ni pending, ni caches d'émission, ni observations, ni banques utilisateur. Les modes S8 SAMPLE/MACHINE sont exclus. Une restauration conserve leur configuration actuelle.

S3 prépare et publie LIVE/NAV ensemble. L'archive LONG demeure dans son autorité existante : un PRESET peut publier GLOBAL pendant LONG SAMPLE, puis le vrai UP retrouve le contexte antérieur sans modifier les valeurs rappelées (Q11). Aucun service DJ/track n'est déclenché par le rappel (Q10). Les témoins valid PRESET suivent leur cycle vivant ; aucun valid historique n'est recopié. Q12 compare les19 composantes natives sur causes physiques note-equivalent et qualifications NAV ; CC/fader/filter seuls, RED direct et applications internes ne l'invalident pas.

La restauration RED annule tous les MUTE en attente, y compris ceux précédant sa capture. Les autres demandes musicales vivantes gardent leur propre échéance. Les snapshots ne ressuscitent aucun pending. INIT et PRESET utilisent l'ordonnanceur déjà présent, avec BEAT/BAR et ordre d'admission distincts ; une préparation temporairement indisponible conserve son ordre.

## Sortie et sûreté
Les valeurs connues produisent leurs CC/notes S7 ordinaires. Un UNKNOWN reste UNKNOWN et n'émet aucune valeur de remplacement. Le facteur LOOP m reste interne ; seuls les trois gains effectifs sont émis. Aucun opcode PRESET ni service externe supposé réussi.

Un rappel global remplace la projection absolue obsolète, même si sa file est pleine, et conserve les demandes musicales futures. Avant les nouvelles activations, il conserve et ferme les NoteOff dus, notamment ceux des TOUCH capturés. Un échec de callback bloque les messages suivants jusqu'au render suivant ; STOP conserve les dettes S4. Le contrôle RED n'a pas besoin d'une place dans la file d'ordonnancement.

La capacité du scheduler couvre les28 pages MEMORY, quatre sources, PRESET, INIT et le domaine d'alternatives. La file nominale et le staging global sont bornés à2048 messages pour les CC/états et leurs OFF de fermeture. Ce n'est pas un transport MIDI privé ni un ACK matériel.

## Preuves et reste
64 suites C++ locales et trois contrôles d'intégrité PASS au premier raccordement. Tests dédiés : quatre slots physiques, RED court/long/STOP, UNKNOWN, modes S8 exclus, Q11, Q12, RED/MUTE des deux côtés de la capture, conservation d'une demande source, ordre INIT/PRESET et projection saturée. Le banc AU macOS ajoute RED physique et l'échec callback/reprise ; son résultat doit être lu dans la CI Apple.

Le raccordement des alternatives ci-dessous complète ce même Bloc3. Le feedback complet et la persistance étendue restent au Bloc4 ; aucune recette AUM/iPhone n'est déduite de ces tests.

## Alternatives S8 raccordées
L'arbitre existant gère deux pools indépendants (INST et YELLOW), chacun avec quatre identifiants instrumentaux natifs0..3. Ces identifiants ne sont ni des numéros de tranche AUM, ni des canaux, ni des programmes. La table groupe→emplacement→identifiant est explicitement configurée, initialement absente. Chaque instrument conserve sa BASE connue, ses trois PATCH60 et ses24 modes séparés. Aucun import de la banque legacy ni configuration fictive.

Dans PRESET, la troisième rangée consulte INST1/INST2/YELLOW1/YELLOW2 ; la quatrième choisit un emplacement configuré de ce groupe. Dernière tranche physique pertinente retenue, UNKNOWN au démarrage ; pas de réorientation par timers, retours ou UP tardifs. Le propriétaire de la sélection est capturé au DOWN, même si un autre groupe est consulté avant UP. Les longs n'ajoutent pas d'action. SAMPLE/DRUMS/FILL restent des destinations fixes et ne reçoivent pas cet éditeur.

Un choix manuel s'autorise au BAR (ou à l'arrêt explicite), ferme les OFF dus sur l'ancienne cible puis publie ACTIVE avec sa BASE et les seuls37 champs SOCLE locaux. Échec d'émission OFF : ACTIVE et LIVE antérieurs conservés ; reprise au render suivant. Les nouvelles commandes de cette source sont bloquées durant la fermeture, les vrais UP restent des nettoyages. STOP révoque la bascule et les callbacks anciens n'ont plus d'autorité.

PRESET/RED capturent les identifiants actifs et leurs états ; restauration sans BASE manuelle supplémentaire. Un snapshot incohérent ou visant un profil local indisponible est refusé avant mutation. Les modes actuels du S8 sélectionné restent en place, y compris UNKNOWN : aucun recours au mode legacy d'un autre instrument. PATCH/CUE/LOAD accèdent à la banque de l'instrument actif ; changer de sélection ne l'efface pas.

L'ordre d'admission est explicite dans le scheduler existant : une annulation pendant un callback ne saute pas le BEAT indépendant suivant ; un report n'est pas consommé deux fois au même edge. Un PRESET retire les sélections plus anciennes qu'il remplace et conserve une sélection indépendante admise plus récemment. Aucun pending n'est restauré depuis le snapshot.

Références ciblées : décision PAIRES_MACHINE_PATCH du30/09, interface S8 L05 (records60 sans modes), sélection destinataire PRESET du19/10, arbitrages C01/C03/C04. API AU : [documentation Apple du paramétrage](https://developer.apple.com/documentation/audiotoolbox/auparameternode/implementorvalueobserver?language=objc).

La confirmation utilisée ici est celle du callback MIDI du plugin. Elle ne prouve ni réception instrumentale, ni changement de programme, ni activation d'un nœud AUM. Leur adaptation reste distincte de cette exécution native universelle.
