// The host does not have this class: a module constructing it must fail to link.
extern class Unlinked
{
   public function new();
}

class ClientUnlinked
{
   public static function main()
   {
      new Unlinked();
   }
}
