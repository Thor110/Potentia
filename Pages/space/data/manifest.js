// Written by tools/update_data.py; do not edit by hand.
// What each dataset was when last fetched: its size, SHA-256, where it came from, and the date it last changed.
(window.OLAP = window.OLAP || {}).manifest = {
  "datasets": {
    "2mrs": {
      "rows": 44599,
      "sha256": "4c2f00deff986c1cebd79223aa941bb84742a8c2ce424336ee4235109f5190a5",
      "via": "https://vizier.cfa.harvard.edu",
      "query": "/viz-bin/asu-tsv?-source=J/ApJS/199/26/table3&-out.max=unlimited&-out=ID,GLON,GLAT,Kcmag,cz",
      "format": "vizier",
      "retrieved": "2026-11-06"
    },
    "localgroup": {
      "rows": 102,
      "sha256": "60b200c49ebbe12b4b4670307bfbeac6f762b4006e918c95445a8db488ac6944",
      "via": "https://vizier.cfa.harvard.edu",
      "query": "/viz-bin/asu-tsv?-source=J/AJ/144/4/catalog&-out.max=unlimited&-out=Name,SubG,MType,GLON,GLAT,D,HRV,VMag",
      "format": "vizier",
      "retrieved": "2026-11-06"
    },
    "exoplanets": {
      "rows": 6436,
      "sha256": "8408eaebabb5378544db1bf556d376a50c7f66b2cf00c406cc3518ea3839174e",
      "bytes": 647486,
      "url": "https://exoplanetarchive.ipac.caltech.edu/TAP/sync?query=select+pl_name,hostname,sy_dist,glon,glat,pl_rade,pl_bmasse,pl_insol,pl_eqt,disc_year+from+pscomppars+where+sy_dist+is+not+null+order+by+sy_dist,pl_name&format=csv",
      "format": "csv",
      "retrieved": "2026-11-06"
    },
    "land": {
      "rows": 127,
      "sha256": "9e0729ee253ca7d7a5c4ae9395fb1902264c5377c52e224d13dd85010e2835d9",
      "bytes": 138160,
      "url": "https://raw.githubusercontent.com/nvkelso/natural-earth-vector/v5.1.2/geojson/ne_110m_land.geojson",
      "format": "geojson",
      "retrieved": "2026-11-06"
    },
    "skin-moon": {
      "bytes": 139068,
      "size": [
        1024,
        512
      ],
      "sha256": "e0721e3bae35f52228f482d5c8d4aa5f0d4052f4ed4913b44cab03356ed86567",
      "source_sha256": "b246064f217f8d479df78c49c7c8595a8f5fbda008a72fd539978d2e121e0109",
      "url": "https://svs.gsfc.nasa.gov/vis/a000000/a004700/a004720/lroc_color_poles_1k.jpg",
      "format": "image",
      "retrieved": "2026-11-06"
    },
    "skin-mars": {
      "bytes": 158789,
      "size": [
        960,
        480
      ],
      "sha256": "fb6c585190d60c2d4b09ef270a7dc89d689175c6b0ab78fa787f4d415a6bd4cf",
      "source_sha256": "80f6c913da5db5619b0946a4525ea4722b20b7e5a7b213363fcf5ef22340d9e7",
      "url": "https://upload.wikimedia.org/wikipedia/commons/thumb/1/1f/Mars_Viking_MDIM21_ClrMosaic_1km.jpg/960px-Mars_Viking_MDIM21_ClrMosaic_1km.jpg",
      "format": "image",
      "retrieved": "2026-11-06"
    },
    "skin-ceres": {
      "bytes": 189530,
      "size": [
        960,
        489
      ],
      "sha256": "1c5860b1d5a840539af7af7ad89912ffb70d7c0e4400fc5ed932eff56403b7e6",
      "source_sha256": "f0cfa26e03a887366f5a888a97086cd571965beb2cad54ff58fc93c7218de508",
      "url": "https://upload.wikimedia.org/wikipedia/commons/thumb/6/67/PIA19606-Ceres-Dawn-GlobalMap-Unannotated-20150728.jpg/960px-PIA19606-Ceres-Dawn-GlobalMap-Unannotated-20150728.jpg",
      "format": "image",
      "retrieved": "2026-11-06"
    }
  }
};
