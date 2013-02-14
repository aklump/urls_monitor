/**
 * @file
 * The main javascript file for urls_monitor
 *
 * @ingroup urls_monitor
 * @{
 */

(function ($) {

  var UrlsMonitor = UrlsMonitor || {};

  /**
   * Check a host and refresh it's row
   *
   * @param string host
   * @param element $row
   */
  UrlsMonitor.check = function(host, $row) {
    $row.addClass('checking');
    $.getJSON('ajax.php', {op: 'urls_monitor_check', host: host}, function(data) {
      UrlsMonitor.refreshRow($row, data);
    });
  }

  /**
   * Update a row's html to reflect new data
   *
   * @param element $row
   * @param object data
   *
   * @see UrlsMonitor.check()
   */
  UrlsMonitor.refreshRow = function($row, data) {
    $.extend(data, {op: 'urls_monitor_theme_tr'});
    $.post('ajax.php', data, function(data) {
      $row.replaceWith(data);
      UrlsMonitor.attachBehaviors();
    })
  }

  /**
   * Attach all behaviors to DOM elements
   *
   * Place click handlers,etc here. Call this after $.ready as well as after
     ajax calls to update the DOM
   */
  UrlsMonitor.attachBehaviors = function() {

    // Click handler for ajax checking
    $('a.ajax-check').click(function() {
      $row = $(this).parents('tr');
      var data = UrlsMonitor.check($(this).attr('rel'), $row);
    });
  }

  /**
  * Core behavior for urls_monitor.
  */
  $(document).ready(function() {

    // Ajax message
    $(document).ajaxStart(function() {
      $('#loading').show();
    });
    $(document).ajaxStop(function() {
      $('#loading').hide();
    });
    UrlsMonitor.attachBehaviors();

    // Fire all click handlers on load
    $('a.ajax-check').each(function() {
      $(this).click();
    });
  });

  /**
  * @} End of "defgroup urls_monitor".
  */

})(jQuery);
