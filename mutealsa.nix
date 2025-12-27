{ config, pkgs, ... }:
let
  mutealsa-bin = pkgs.fetchurl {
    url = "https://github.com/FinlayTheBerry/mutealsa/releases/download/v0.1.0/mutealsa";
    sha256 = "HASH"; 
    executable = true;
  };
in
{
  systemd.services.mutealsa-service = {
    description = "Runs mutealsa on sleep/hibernate.";
    wantedBy = [ 
      "sleep.target" 
      "suspend.target" 
      "hibernate.target" 
      "hybrid-sleep.target" 
      "suspend-then-hibernate.target" 
    ];
    before = [ 
      "sleep.target" 
      "suspend.target" 
      "hibernate.target" 
      "hybrid-sleep.target" 
      "suspend-then-hibernate.target" 
    ];
    unitConfig = {
      StopWhenUnneeded = "yes";
    };
    serviceConfig = {
      Type = "oneshot";
      RemainAfterExit = "yes";
      ExecStart = "${mutealsa-bin} pre";
      ExecStop = "${mutealsa-bin} post";
      User = "root";
    };
  };
  environment.systemPackages = [ pkgs.alsa-lib ];
}