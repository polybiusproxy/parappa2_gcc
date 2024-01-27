# CYGNUS LOCAL entire file

if {[istarget "arm-*-*"] || [istarget "thumb-*-*"] || [istarget "strongarm-*-*"]} {

   # On the arm specifying -g produces a bogus label reference
   # in debugging output.

    set torture_eval_before_compile {
      set additional_flags "-g0"
    }
}

return 0
