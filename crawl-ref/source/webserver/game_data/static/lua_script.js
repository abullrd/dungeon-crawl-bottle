define(["jquery", "comm", "./player"], function ($, comm, player) {
    function load_script(e)
    {
        e.preventDefault();
        e.stopPropagation();
	    comm.send_message("load_script", {
           username: player.name,
           script_url: $("#script_git_url" ).val()
	    });
    }

    $(document).ready(function () {
        $("#load_script").bind("click", load_script);
    });
})
