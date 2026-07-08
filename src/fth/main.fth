: white 255 255 255 255 ;
: black 0 0 0 255 ;

: frame ( -- )
  rl.begin-drawing
    black rl.clear-background
    220 160 200 160 white rl.draw-rectangle
  rl.end-drawing ;

: main ( -- )
  640 480 s" forthray" rl.init-window
  50 rl.set-fps

  begin rl.window-should-close 0= while
    frame
  repeat
  rl.close-window ;

main
bye
