; ModuleID = 'originalModule'
source_filename = "originalModule"

declare double @hoge(double)

declare double @_Z8hoge_cppd(double)

declare double @cos(double)

define double @originalFunction(double %a, double %b) {
entry:
  %tmphoge = call double @_Z8hoge_cppd(double %a)
  %tmphoge1 = call double @cos(double %tmphoge)
  %addtmp = fadd double %tmphoge1, %b
  ret double %addtmp
}