# Modified low-delay production bank

SADIE II copyright2018 University of York, Apache License2.0; retain
LICENSE-SADIE-II.txt and NOTICE.md. Original authors: Cal Armstrong, Lewis Thresh,
Gavin Kearney, AudioLab, University of York. Source record10886409 on Zenodo.

HamloShirota further modifications for production: minimum-phase factorization
of the prepared48k magnitude, measured interaural correlation lag restored about
56sample reference,512tap float32 storage. Runtime preparation resamples this
bank to the host rate and retains the fixed Legacy host PDC. Source S3 bank is
unchanged. Tools/prepare_s4_hrtf.py and production.json document transformations,
frequency/ITD errors, tail loss and hashes. These changes are our own renderer
preparation; no personalized headphone correction is implied.
