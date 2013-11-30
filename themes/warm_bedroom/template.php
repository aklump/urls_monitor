<?php
/**
 * @file
 * Functions for the theme
 */

/**
 * Alter the page info before it's output
 *
 * @param  array &$info page info
 */
function warm_bedroom_urls_monitor_page_alter(&$info) {
  $info['head'] .= <<<EOD
<script type="text/javascript">
  var cSpeed=7;
  var cWidth=128;
  var cHeight=128;
  var cTotalFrames=35;
  var cFrameWidth=128;
  var cImageSrc='/themes/warm_bedroom/images/checking.png';
  
  var cImageTimeout=false;
  
  function startAnimation(){
    
    document.getElementById('loader-image').innerHTML='<canvas id="canvas" width="'+cWidth+'" height="'+cHeight+'"><p>Your browser does not support the canvas element.</p></canvas>';
    
    //FPS = Math.round(100/(maxSpeed+2-speed));
    FPS = Math.round(100/cSpeed);
    SECONDS_BETWEEN_FRAMES = 1 / FPS;
    g_GameObjectManager = null;
    g_run=genImage;

    g_run.width=cTotalFrames*cFrameWidth;
    genImage.onload=function (){cImageTimeout=setTimeout(fun, 0)};
    initCanvas();
  }
  
  
  function imageLoader(s, fun)//Pre-loads the sprites image
  {
    clearTimeout(cImageTimeout);
    cImageTimeout=0;
    genImage = new Image();
    genImage.onload=function (){cImageTimeout=setTimeout(fun, 0)};
    genImage.onerror=new Function('alert(\'Could not load the image\')');
    genImage.src=s;
  }
  
  //The following code starts the animation
  new imageLoader(cImageSrc, 'startAnimation()');
</script>
EOD;
}

function warm_bedroom_urls_monitor_preprocess_row_alter(&$row) {

  // Replace the row img tags with theme image files...
  foreach (array_keys($row['data']) as $key) {
    if (is_array($row['data'][$key])) {
      $row['data'][$key]['data'] = str_replace('src="/images/', 'src="/themes/warm_bedroom/images/', $row['data'][$key]['data']);
      $row['data'][$key]['data'] = str_replace('/reload.gif', '/reload.png', $row['data'][$key]['data']);
    }
    else {
      $row['data'][$key] = str_replace('src="/images/', 'src="/themes/warm_bedroom/images/', $row['data'][$key]);
    }
  }
}