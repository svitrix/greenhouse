export function formatIeeeColons(ieee16: string): string {
  return ieee16.match(/.{2}/g)!.join(':');
}
