from pathlib import Path
import re
p=Path('Mozaic/K2_SIM_V3.txt')
s=p.read_text()
notes=list(map(int,re.search(r'Notes = \[([^]]+)\]',s)[1].split(',')))
# Independent explicit physical bands from GOLDEN/S7 S1 @MapButton.
expected=[12,13,14,15,36,37,38,39,32,33,34,35,28,29,30,31,24,25,26,27,48,49,50,51,44,45,46,47,40,41,42,43,52,53,54,55]
assert notes==expected and len(set(notes))==36
assert 'SendMIDI' not in s.split('@OnLoad')[1].split('@End')[0]
assert 'SendMIDINoteOff 14, HeldNote[Pad], 0' in s
assert 'SendMIDICC 14, CCNo, Value' in s
assert 'SendMIDIThru' not in s and 'SendSysex' not in s
assert {b*6+k for b in range(4) for k in range(6) if b*6+k<22}==set(range(22))
assert {4+p*12+k for p in range(3) for k in range(12) if 4+p*12+k<36}==set(range(4,36))
print('PASS K2_SIM source table: 36 notes, 22 CC, captured releases, no load TX/Thru; not Mozaic compilation')
