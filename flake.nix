{
  description = "Video Hotspot — privacy-preserving video upload and geolocation-based event mapping on the Logos stack";

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder";
    logos-standalone-app.url = "github:logos-co/logos-standalone-app";
    # Follow nixpkgs from logos-module-builder for consistency
    nixpkgs.follows = "logos-module-builder/nixpkgs";
  };

  outputs = { logos-module-builder, logos-standalone-app, ... }:
    logos-module-builder.lib.mkLogosModule {
      src = ./.;
      configFile = ./module.yaml;
      # Wire up logos-standalone-app so `nix run` launches the UI module
      logosStandalone = logos-standalone-app;
      iconFiles = [ ];
    };
}
