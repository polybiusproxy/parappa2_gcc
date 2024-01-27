        .SPACE $PRIVATE$
        .SUBSPA $DATA$,QUAD=1,ALIGN=8,ACCESS=31
        .SUBSPA $BSS$,QUAD=1,ALIGN=8,ACCESS=31,ZERO,SORT=82
        .SPACE $TEXT$
        .SUBSPA $LIT$,QUAD=0,ALIGN=8,ACCESS=44
        .SUBSPA $CODE$,QUAD=0,ALIGN=8,ACCESS=44,CODE_ONLY

        .level 2.0
addb_tests
        addb,*= %r1,%r4,addb_tests
        addb,*< %r1,%r4,addb_tests
        addb,*<= %r1,%r4,addb_tests
        addb,*<> %r1,%r4,addb_tests
        addb,*>= %r1,%r4,addb_tests
        addb,*> %r1,%r4,addb_tests
        addb,*=,n %r1,%r4,addb_tests
        addb,*<,n %r1,%r4,addb_tests
        addb,*<=,n %r1,%r4,addb_tests
        addb,*<>,n %r1,%r4,addb_tests
        addb,*>=,n %r1,%r4,addb_tests
        addb,*>,n %r1,%r4,addb_tests
        addib,*= 1,%r4,addb_tests
        addib,*< 1,%r4,addb_tests
        addib,*<= 1,%r4,addb_tests
        addib,*<> 1,%r4,addb_tests
        addib,*>= 1,%r4,addb_tests
        addib,*> 1,%r4,addb_tests
        addib,*=,n 1,%r4,addb_tests
        addib,*<,n 1,%r4,addb_tests
        addib,*<=,n 1,%r4,addb_tests
        addib,*<>,n 1,%r4,addb_tests
        addib,*>=,n 1,%r4,addb_tests
        addib,*>,n 1,%r4,addb_tests



