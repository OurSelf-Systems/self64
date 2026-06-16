 '$Revision: 30.2 $'
 '
Copyright 1992-2012 AUTHORS.
See the LICENSE file for license information.
'


 '-- Module body'

 bootstrap addSlotsTo: bootstrap stub -> 'globals' -> 'modules' -> () From: ( | {
         'ModuleInfo: Module: smallIntTests InitialContents: FollowSlot'
        
         smallIntTests = bootstrap define: bootstrap stub -> 'globals' -> 'modules' -> 'smallIntTests' -> () ToBe: bootstrap addSlotsTo: (
             bootstrap remove: 'directory' From:
             bootstrap remove: 'fileInTimeString' From:
             bootstrap remove: 'myComment' From:
             bootstrap remove: 'postFileIn' From:
             bootstrap remove: 'revision' From:
             bootstrap remove: 'subpartNames' From:
             globals modules init copy ) From: bootstrap setObjectAnnotationOf: bootstrap stub -> 'globals' -> 'modules' -> 'smallIntTests' -> () From: ( |
             {} = 'ModuleInfo: Creator: globals modules smallIntTests.

CopyDowns:
globals modules init. copy 
SlotsToOmit: directory fileInTimeString myComment postFileIn revision subpartNames.

\x7fIsComplete: '.
            | ) .
        } | ) 

 bootstrap addSlotsTo: bootstrap stub -> 'globals' -> 'modules' -> 'smallIntTests' -> () From: ( | {
         'ModuleInfo: Module: smallIntTests InitialContents: FollowSlot\x7fVisibility: public'
        
         directory <- 'core'.
        } | ) 

 bootstrap addSlotsTo: bootstrap stub -> 'globals' -> 'modules' -> 'smallIntTests' -> () From: ( | {
         'ModuleInfo: Module: smallIntTests InitialContents: InitializeToExpression: (_CurrentTimeString)\x7fVisibility: public'
        
         fileInTimeString <- _CurrentTimeString.
        } | ) 

 bootstrap addSlotsTo: bootstrap stub -> 'globals' -> 'modules' -> 'smallIntTests' -> () From: ( | {
         'ModuleInfo: Module: smallIntTests InitialContents: FollowSlot'
        
         myComment <- ''.
        } | ) 

 bootstrap addSlotsTo: bootstrap stub -> 'globals' -> 'modules' -> 'smallIntTests' -> () From: ( | {
         'ModuleInfo: Module: smallIntTests InitialContents: FollowSlot'
        
         postFileIn = ( |
            | resend.postFileIn).
        } | ) 

 bootstrap addSlotsTo: bootstrap stub -> 'globals' -> 'modules' -> 'smallIntTests' -> () From: ( | {
         'ModuleInfo: Module: smallIntTests InitialContents: FollowSlot\x7fVisibility: public'
        
         revision <- '$Revision: 30.2 $'.
        } | ) 

 bootstrap addSlotsTo: bootstrap stub -> 'globals' -> 'modules' -> 'smallIntTests' -> () From: ( | {
         'ModuleInfo: Module: smallIntTests InitialContents: FollowSlot\x7fVisibility: private'
        
         subpartNames <- ''.
        } | ) 

 bootstrap addSlotsTo: bootstrap stub -> 'traits' -> 'smallInt' -> () From: ( | {
         'Category: unit tests\x7fModuleInfo: Module: smallIntTests InitialContents: FollowSlot\x7fVisibility: private'
        
         unitTests = bootstrap setObjectAnnotationOf: bootstrap stub -> 'traits' -> 'smallInt' -> 'unitTests' -> () From: ( |
             {} = 'ModuleInfo: Creator: traits smallInt unitTests.
\x7fIsComplete: '.
            | ) .
        } | ) 

 bootstrap addSlotsTo: bootstrap stub -> 'traits' -> 'smallInt' -> 'unitTests' -> () From: ( | {
         'ModuleInfo: Module: smallIntTests InitialContents: FollowSlot\x7fVisibility: private'
        
         parent* = bootstrap setObjectAnnotationOf: bootstrap stub -> 'traits' -> 'smallInt' -> 'unitTests' -> 'parent' -> () From: ( |
             {} = 'ModuleInfo: Creator: traits smallInt unitTests parent.
'.
            | ) .
        } | ) 

 bootstrap addSlotsTo: bootstrap stub -> 'traits' -> 'smallInt' -> 'unitTests' -> 'parent' -> () From: ( | {
         'Category: assertions\x7fModuleInfo: Module: smallIntTests InitialContents: FollowSlot\x7fVisibility: private'
        
         assert: blk = ( |
            | 
            blk value ifFalse: [error: 'smallInt test failed'].
            self).
        } | ) 

 bootstrap addSlotsTo: bootstrap stub -> 'traits' -> 'smallInt' -> 'unitTests' -> 'parent' -> () From: ( | {
         'Category: assertions\x7fModuleInfo: Module: smallIntTests InitialContents: FollowSlot\x7fVisibility: private'
        
         assert: a Is: b = ( |
            | 
            "This method is just useful because you
             can click on a and b right in the debugger
             to see what the two values are."
            assert: [a = b]).
        } | ) 

 bootstrap addSlotsTo: bootstrap stub -> 'traits' -> 'smallInt' -> 'unitTests' -> 'parent' -> () From: ( | {
         'Category: assertions\x7fModuleInfo: Module: smallIntTests InitialContents: FollowSlot\x7fVisibility: private'
        
         assertFail: blk = ( |
            | 
            blk value: [^ self].
            fail).
        } | ) 

 bootstrap addSlotsTo: bootstrap stub -> 'traits' -> 'smallInt' -> 'unitTests' -> 'parent' -> () From: ( | {
         'Category: assertions\x7fModuleInfo: Module: smallIntTests InitialContents: FollowSlot\x7fVisibility: private'
        
         fail = ( |
            | 
            assert: [false]).
        } | ) 

 bootstrap addSlotsTo: bootstrap stub -> 'traits' -> 'smallInt' -> 'unitTests' -> 'parent' -> () From: ( | {
         'ModuleInfo: Module: smallIntTests InitialContents: FollowSlot\x7fVisibility: private'
        
         parent* = bootstrap stub -> 'traits' -> 'clonable' -> ().
        } | ) 

 bootstrap addSlotsTo: bootstrap stub -> 'traits' -> 'smallInt' -> 'unitTests' -> 'parent' -> () From: ( | {
         'ModuleInfo: Module: smallIntTests InitialContents: FollowSlot\x7fVisibility: public'
        
         run = ( |
            | 
            testSuccess.
            testFailure.
            self).
        } | ) 

 bootstrap addSlotsTo: bootstrap stub -> 'traits' -> 'smallInt' -> 'unitTests' -> 'parent' -> () From: ( | {
         'ModuleInfo: Module: smallIntTests InitialContents: FollowSlot\x7fVisibility: private'
        
         testFailure = ( |
            | 
            assertFail: [|:fb| maxSmallInt _IntAdd:     1 IfFail: fb].
            assertFail: [|:fb| minSmallInt _IntSub:     1 IfFail: fb].
            "overflow triggers are expressed relative to the VM's smi width
             (maxSmallInt/minSmallInt) so they fail on any word size, rather
             than assuming a 30-bit smallInt -- rca 2026"
            assertFail: [|:fb| maxSmallInt _IntMul:     2 IfFail: fb].
            assertFail: [|:fb|          42 _IntDiv:     0 IfFail: fb].
            assertFail: [|:fb|          42 _IntMod:     0 IfFail: fb].

            assertFail: [|:fb| minSmallInt _IntArithmeticShiftLeft: 1 IfFail: fb].
            assertFail: [|:fb| maxSmallInt _IntArithmeticShiftLeft: 1 IfFail: fb].

            [assertFail: [|:fb| -1 _IntArithmeticShiftLeft: 64 IfFail: fb]]. [todo selfBug dave]. "Bug in Self."
            self).
        } | ) 

 bootstrap addSlotsTo: bootstrap stub -> 'traits' -> 'smallInt' -> 'unitTests' -> 'parent' -> () From: ( | {
         'ModuleInfo: Module: smallIntTests InitialContents: FollowSlot\x7fVisibility: private'
        
         testSuccess = ( |
            | 
            [todo testing]. "Add more exceptional cases -- jb 7/03"

            assert: [ 5  = 5].
            assert: [(5  = 4) not].
            assert: [ 5 != 4].
            assert: [(5 != 5) not].

            assert: [  4 <=  5].
            assert: [( 5 <=  4) not].
            assert: [ -4 <=  5].
            assert: [(-4 <= -5) not].

            assert: [  4 <   5].
            assert: [( 4 <   4) not].
            assert: [( 5 <   4) not].
            assert: [(-4 <  -5) not].

            assert: [  5 >   4].
            assert: [( 5 >   5) not].
            assert: [( 4 >   5) not].
            assert: [(-5 >  -4) not].

            assert: [  5 >=  4].
            assert: [  5 >=  5].
            assert: [( 4 >=  5) not].
            assert: [(-5 >= -4) not].

            assert:   5 +  4 Is:   9.
            assert:  30 +  3 Is:  33.
            assert:  30 + -3 Is:  27.
            assert: -30 +  3 Is: -27.
            assert: -30 + -3 Is: -33.

            assert:   5 -  4 Is:   1.
            assert:  30 -  3 Is:  27.
            assert:  30 - -3 Is:  33.
            assert: -30 -  3 Is: -33.
            assert: -30 - -3 Is: -27.

            assert:   5 *  4 Is:  20.
            assert:  30 *  3 Is:  90.
            assert:  30 * -3 Is: -90.
            assert: -30 *  3 Is: -90.
            assert: -30 * -3 Is:  90.

            assert:   5 /  4 Is:   1.
            assert:  30 /  3 Is:  10.
            assert:  30 / -3 Is: -10.
            assert: -30 /  3 Is: -10.
            assert: -30 / -3 Is:  10.

            [todo selfBug dave]. "This is a bug in Self."
            "assert: -16r20000000 / -1 Is: 16r1FFFFFFF + 1."

            assert: 19 % 7 Is: 5.
            assert: 21 % 7 Is: 0.
            assert: 21 % 1 Is: 0.

            assert:  ( 5 <+ 4) Is:   80.
            assert:  (30 <+ 3) Is:  240.
            assert: ( -5 <+ 4) Is:  -80.
            assert: (-30 <+ 3) Is: -240.

            assert: (  5 +> 4) Is:  0.
            assert: ( 30 +> 3) Is:  3.
            assert: ( -5 +> 4) Is: -1.
            assert: (-30 +> 3) Is: -4.

            assert:   5 << 4 Is:   80.
            assert:  30 << 3 Is:  240.
            assert:  -5 << 4 Is:  -80.
            assert: -30 << 3 Is: -240.

            assert:   5 >> 4 Is:         0.
            assert:  30 >> 3 Is:         3.
            "logical right shift of a negative depends on the smi width, so
             express the expected value relative to maxSmallInt (= -1 >> 1)
             rather than assuming a 30-bit smallInt -- rca 2026"
            assert:  -5 >> 4 Is: (maxSmallInt >> 3).
            assert: -30 >> 3 Is: ((maxSmallInt >> 2) - 3).

            assert: 1  && 1 Is: 1.
            assert: 14 && 6 Is: 6.
            assert: 14 && 7 Is: 6.
            assert: 0  && 7 Is: 0.
            assert: -5 && 7 Is: 3.

            assert: 1  || 1 Is:  1.
            assert: 14 || 7 Is: 15.
            assert: 0  || 5 Is:  5.

            assert: 1  ^^ 1 Is:  0.
            assert: 14 ^^ 7 Is:  9.
            assert: 0  ^^ 5 Is:  5.

            assert:     5 _IntComplement Is:   -6.
            assert: -5000 _IntComplement Is: 4999.

            self).
        } | ) 



 '-- Side effects'

 globals modules smallIntTests postFileIn
