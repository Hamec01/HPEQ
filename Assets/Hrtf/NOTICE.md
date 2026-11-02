# SADIE II horizontal prototype data

Copyright 2018 University of York. Licensed under Apache License, Version2.0;
see LICENSE-SADIE-II.txt (the exact upstream license).
Recorded/developed by Cal Armstrong, Lewis Thresh and Gavin Kearney, AudioLab,
Department of Electronic Engineering, University of York, UK.
Source: https://zenodo.org/records/10886409 (SADIE II v2-1).
Publication: https://doi.org/10.3390/app8112029 .

HamloShirota modifications: select the horizontal plane every5degrees from D1,
H3 and H4, use the first regular-grid occurrence where SOFA includes repeated
directions, apply one common gain per profile, and pack all native FIR taps into
float32. There is no tail truncation, phase conversion, resampling, headphone
correction or room response. Source archives, SOFA hashes, embedded notices,
measurement indices, gains, transformation and fidelity checks: manifest.json.
Renderer and path implementation are our own. This offline bank is not yet
included in a shipping plug-in; carry this notice/license into any later package.
