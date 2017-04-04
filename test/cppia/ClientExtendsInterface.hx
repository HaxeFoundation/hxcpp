class ClientExtendsInterface extends ClientExtends implements ClientInterface {
  public function isOk():Bool {
    return this.ok();
  }
}

interface ClientInterface {
  public function isOk():Bool;
}

