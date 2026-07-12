# Card art generation prompts — 3 batches of 12

Generate each batch as its own image. Paste the ENTIRE batch block (style
header + card list) into Gemini each time — the repeated style header is
what keeps the three sheets visually consistent. Save each result and give
Claude the three file paths; slicing/import is automated from there.

---

## BATCH 1 of 3

Create a single high-resolution sprite sheet: a 6-column by 2-row grid of
12 fantasy playing cards for a retro pixel-art card game, on a plain solid
dark gray background with clear uniform spacing between cards.

STYLE (applies to every card): 16-bit pixel art, dark-fantasy
dungeon-crawler aesthetic inspired by Slay the Spire. Every card is
identical in size, portrait orientation, standard playing-card proportions
(5:7 ratio). Each card has: a thick colored outer frame, a parchment name
banner across the top with the card's name in bold blocky pixel lettering,
a round energy-cost orb in the top-left corner overlapping the frame with a
large white number, and a central illustration filling the rest of the
card. Chunky pixels, dark outlines, high contrast, limited palette per card
(10-14 colors). Illustrations are bold and readable even when shrunk small:
one clear subject per card, no fine detail, no gradients.

FRAME COLORS BY CARD TYPE: Attack cards = deep red frame. Skill cards =
forest green frame. Power cards = teal/cyan frame. Status cards = dull gray
frame with a sickly look.

THE 12 CARDS, in exact grid order (left to right, top to bottom):

Row 1: STRIKE (attack, cost 1, a sword slashing diagonally with orange
energy trail) • DEFEND (skill, cost 1, a sturdy kite shield) • BASH
(attack, cost 2, a spiked shield smashing with impact burst) • ANGER
(attack, cost 0, a furious red demon face screaming) • BODY SLAM (attack,
cost 1, an armored warrior shoulder-charging) • CLEAVE (attack, cost 1, a
wide horizontal sword arc hitting multiple shadows)

Row 2: CLOTHESLINE (attack, cost 2, a muscular outstretched arm striking a
neck) • FLEX (skill, cost 0, a flexing bicep glowing red) • HEADBUTT
(attack, cost 1, a helmeted head striking forward with stars) • HEAVY BLADE
(attack, cost 2, a massive two-handed greatsword crashing down) • IRON WAVE
(attack, cost 1, a sword strike and shield raised together in one motion) •
POMMEL STRIKE (attack, cost 1, a sword pommel jabbing forward with cards
flying behind)

Every card must be fully inside its grid cell with nothing overlapping.
Same lighting, same pixel density, same rendering style across all 12
cards.

---

## BATCH 2 of 3

Create a single high-resolution sprite sheet: a 6-column by 2-row grid of
12 fantasy playing cards for a retro pixel-art card game, on a plain solid
dark gray background with clear uniform spacing between cards.

STYLE (applies to every card): 16-bit pixel art, dark-fantasy
dungeon-crawler aesthetic inspired by Slay the Spire. Every card is
identical in size, portrait orientation, standard playing-card proportions
(5:7 ratio). Each card has: a thick colored outer frame, a parchment name
banner across the top with the card's name in bold blocky pixel lettering,
a round energy-cost orb in the top-left corner overlapping the frame with a
large white number, and a central illustration filling the rest of the
card. Chunky pixels, dark outlines, high contrast, limited palette per card
(10-14 colors). Illustrations are bold and readable even when shrunk small:
one clear subject per card, no fine detail, no gradients.

FRAME COLORS BY CARD TYPE: Attack cards = deep red frame. Skill cards =
forest green frame. Power cards = teal/cyan frame. Status cards = dull gray
frame with a sickly look.

THE 12 CARDS, in exact grid order (left to right, top to bottom):

Row 1: SHRUG IT OFF (skill, cost 1, a warrior shrugging off a glancing
arrow with a faint shield glow) • THUNDERCLAP (attack, cost 1, two fists
clapping with lightning shockwave) • TWIN STRIKE (attack, cost 1, two
parallel crossing sword slashes) • BLOODLETTING (skill, cost 0, a dagger
cutting a palm with three drops of blood and yellow energy rising) •
CARNAGE (attack, cost 2, a brutal cleaver dripping blood, edges of the card
dissolving into mist) • DISARM (skill, cost 1, a sword being knocked out of
a monster's grip)

Row 2: ENTRENCH (skill, cost 2, a shield doubled/mirrored behind itself
glowing) • FLAME BARRIER (skill, cost 2, a wall of fire encircling a
shield) • INFLAME (power, cost 1, a heart engulfed in rising flame) •
METALLICIZE (power, cost 1, skin turning to metal plates) • SHOCKWAVE
(skill, cost 2, a radial stun ring knocking back two silhouettes) •
UPPERCUT (attack, cost 2, a rising armored fist launching a jaw upward)

Every card must be fully inside its grid cell with nothing overlapping.
Same lighting, same pixel density, same rendering style across all 12
cards.

---

## BATCH 3 of 3

Create a single high-resolution sprite sheet: a 6-column by 2-row grid of
12 fantasy playing cards for a retro pixel-art card game, on a plain solid
dark gray background with clear uniform spacing between cards.

STYLE (applies to every card): 16-bit pixel art, dark-fantasy
dungeon-crawler aesthetic inspired by Slay the Spire. Every card is
identical in size, portrait orientation, standard playing-card proportions
(5:7 ratio). Each card has: a thick colored outer frame, a parchment name
banner across the top with the card's name in bold blocky pixel lettering,
a round energy-cost orb in the top-left corner overlapping the frame with a
large white number, and a central illustration filling the rest of the
card. Chunky pixels, dark outlines, high contrast, limited palette per card
(10-14 colors). Illustrations are bold and readable even when shrunk small:
one clear subject per card, no fine detail, no gradients.

FRAME COLORS BY CARD TYPE: Attack cards = deep red frame. Skill cards =
forest green frame. Power cards = teal/cyan frame. Status cards = dull gray
frame with a sickly look.

THE 12 CARDS, in exact grid order (left to right, top to bottom):

Row 1: WHIRLWIND (attack, cost X — show a stylized "X" on the orb, a
spinning blade tornado hitting all around) • BARRICADE (power, cost 3, a
massive stone-and-iron wall with a shield emblem) • BLUDGEON (attack, cost
3, an enormous spiked warhammer cracking the ground) • DEMON FORM (power,
cost 3, a warrior silhouette growing demon horns and red aura) • FEED
(attack, cost 1, a dark maw devouring a red heart) • IMMOLATE (attack, cost
2, a pillar of flame consuming everything, embers falling)

Row 2: IMPERVIOUS (skill, cost 2, a warrior inside a glowing translucent
diamond barrier) • OFFERING (skill, cost 0, a bleeding hand held over a
ritual bowl with rising cards) • WOUND (status, no cost orb, a jagged open
gash, cracked card face) • BURN (status, no cost orb, the card itself on
fire, charred edges) • SLIMED (status, cost 1, dripping green goo covering
the card face) • DAZED (status, no cost orb, dizzy swirl stars on a
translucent ghostly card)

Every card must be fully inside its grid cell with nothing overlapping.
Same lighting, same pixel density, same rendering style across all 12
cards.

---

## Slicing reference (Claude uses this on import)

Batch 1 → `strike defend bash anger bodyslam cleave / clothesline flex
headbutt heavyblade ironwave pommel`
Batch 2 → `shrug thunderclap twinstrike bloodlet carnage disarm / entrench
flamebarrier inflame metallicize shockwave uppercut`
Batch 3 → `whirlwind barricade bludgeon demonform feed immolate /
impervious offering wound burn slimed dazed`

Target in-game size: 40x56 px (5:7). Each card auto-quantized to ≤15
colors. Provide the three sheet file paths to Claude when generated.
